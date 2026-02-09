#include "hormann_bisecur_protocol.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/devices/devices.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>

#define HORMANN_FREQ 868300000

// BiSecur frame constants (placeholders - refine from captured signals)
#define HORMANN_PREAMBLE_BYTE  0xAA
#define HORMANN_PREAMBLE_LEN   4
#define HORMANN_SYNC_WORD_HI   0x2D
#define HORMANN_SYNC_WORD_LO   0xD4
#define HORMANN_AES_BLOCK_SIZE 16
#define HORMANN_BIT_DURATION   208 // microseconds (~4.8 kbps)
#define HORMANN_REPEAT_COUNT   3
#define HORMANN_INTER_FRAME_US 20000

// CRC-16 CCITT (poly 0x1021, init 0xFFFF)
static uint16_t hormann_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for(size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for(uint8_t j = 0; j < 8; j++) {
            if(crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

// Full frame layout:
// [4 bytes preamble] [2 bytes sync] [16 bytes AES ciphertext] [2 bytes CRC]
// Total: 24 bytes = 192 bits
#define HORMANN_FRAME_SIZE 24

typedef enum {
    TxStatePreamble,
    TxStateSyncWord,
    TxStateData,
    TxStateCRC,
    TxStateGap,
    TxStateDone,
} TxState;

typedef struct {
    uint8_t frame[HORMANN_FRAME_SIZE];
    TxState state;
    uint8_t repeat;
    uint16_t bit_idx;
} HormannTxState;

// AES-128 ECB encrypt a single 16-byte block using hardware AES.
// The HAL expects a 32-byte key (AES-256), so we zero-pad our 16-byte key.
// With a zero IV, CBC on a single block is equivalent to ECB.
static void hormann_aes128_ecb_encrypt(
    const uint8_t* key_16,
    const uint8_t* plaintext,
    uint8_t* ciphertext) {
    uint8_t key_32[32];
    memset(key_32, 0, sizeof(key_32));
    memcpy(key_32, key_16, 16);

    uint8_t iv[16];
    memset(iv, 0, sizeof(iv));

    furi_hal_crypto_load_key(key_32, iv);
    furi_hal_crypto_encrypt(plaintext, ciphertext, HORMANN_AES_BLOCK_SIZE);
    furi_hal_crypto_unload_key();
}

static void hormann_bisecur_build_frame(
    uint8_t* frame,
    uint32_t serial,
    const uint8_t* aes_key,
    uint32_t counter,
    HormannCmd cmd) {
    // Build plaintext payload (16 bytes)
    uint8_t plaintext[HORMANN_AES_BLOCK_SIZE];
    memset(plaintext, 0, sizeof(plaintext));

    // Bytes 0-3: Serial (big-endian)
    plaintext[0] = (serial >> 24) & 0xFF;
    plaintext[1] = (serial >> 16) & 0xFF;
    plaintext[2] = (serial >> 8) & 0xFF;
    plaintext[3] = serial & 0xFF;

    // Bytes 4-7: Counter (big-endian)
    plaintext[4] = (counter >> 24) & 0xFF;
    plaintext[5] = (counter >> 16) & 0xFF;
    plaintext[6] = (counter >> 8) & 0xFF;
    plaintext[7] = counter & 0xFF;

    // Byte 8: Command
    plaintext[8] = (uint8_t)cmd;

    // Bytes 9-15: Reserved (zeros)

    // AES-128 ECB encrypt
    uint8_t ciphertext[HORMANN_AES_BLOCK_SIZE];
    hormann_aes128_ecb_encrypt(aes_key, plaintext, ciphertext);

    // Assemble frame
    uint8_t pos = 0;

    // Preamble
    for(uint8_t i = 0; i < HORMANN_PREAMBLE_LEN; i++) {
        frame[pos++] = HORMANN_PREAMBLE_BYTE;
    }

    // Sync word
    frame[pos++] = HORMANN_SYNC_WORD_HI;
    frame[pos++] = HORMANN_SYNC_WORD_LO;

    // Encrypted payload
    memcpy(&frame[pos], ciphertext, HORMANN_AES_BLOCK_SIZE);
    pos += HORMANN_AES_BLOCK_SIZE;

    // CRC-16 over ciphertext
    uint16_t crc = hormann_crc16(ciphertext, HORMANN_AES_BLOCK_SIZE);
    frame[pos++] = (crc >> 8) & 0xFF;
    frame[pos++] = crc & 0xFF;
}

static LevelDuration hormann_tx_callback(void* context) {
    HormannTxState* tx = context;

    switch(tx->state) {
    case TxStatePreamble: {
        uint8_t byte_idx = tx->bit_idx / 8;
        uint8_t bit_pos = 7 - (tx->bit_idx % 8);
        bool level = (tx->frame[byte_idx] >> bit_pos) & 1;
        tx->bit_idx++;
        if(tx->bit_idx >= HORMANN_PREAMBLE_LEN * 8) {
            tx->state = TxStateSyncWord;
            tx->bit_idx = 0;
        }
        return level_duration_make(level, HORMANN_BIT_DURATION);
    }

    case TxStateSyncWord: {
        uint8_t offset = HORMANN_PREAMBLE_LEN;
        uint8_t byte_idx = offset + tx->bit_idx / 8;
        uint8_t bit_pos = 7 - (tx->bit_idx % 8);
        bool level = (tx->frame[byte_idx] >> bit_pos) & 1;
        tx->bit_idx++;
        if(tx->bit_idx >= 16) {
            tx->state = TxStateData;
            tx->bit_idx = 0;
        }
        return level_duration_make(level, HORMANN_BIT_DURATION);
    }

    case TxStateData: {
        uint8_t offset = HORMANN_PREAMBLE_LEN + 2;
        uint8_t byte_idx = offset + tx->bit_idx / 8;
        uint8_t bit_pos = 7 - (tx->bit_idx % 8);
        bool level = (tx->frame[byte_idx] >> bit_pos) & 1;
        tx->bit_idx++;
        if(tx->bit_idx >= HORMANN_AES_BLOCK_SIZE * 8) {
            tx->state = TxStateCRC;
            tx->bit_idx = 0;
        }
        return level_duration_make(level, HORMANN_BIT_DURATION);
    }

    case TxStateCRC: {
        uint8_t offset = HORMANN_PREAMBLE_LEN + 2 + HORMANN_AES_BLOCK_SIZE;
        uint8_t byte_idx = offset + tx->bit_idx / 8;
        uint8_t bit_pos = 7 - (tx->bit_idx % 8);
        bool level = (tx->frame[byte_idx] >> bit_pos) & 1;
        tx->bit_idx++;
        if(tx->bit_idx >= 16) {
            tx->state = TxStateGap;
            tx->bit_idx = 0;
        }
        return level_duration_make(level, HORMANN_BIT_DURATION);
    }

    case TxStateGap:
        tx->repeat++;
        if(tx->repeat >= HORMANN_REPEAT_COUNT) {
            tx->state = TxStateDone;
        } else {
            tx->state = TxStatePreamble;
            tx->bit_idx = 0;
        }
        return level_duration_make(false, HORMANN_INTER_FRAME_US);

    case TxStateDone:
    default:
        return level_duration_reset();
    }
}

// Custom CC1101 register preset for 2-FSK at 868.3 MHz, ~4.8 kbps, ~5 kHz deviation
static const uint8_t hormann_cc1101_regs[] = {
    // IOCFG0 - GDO0 output for async TX
    0x00, 0x0D,
    // FIFOTHR
    0x03, 0x07,
    // PKTCTRL0 - async serial mode, no CRC
    0x08, 0x32,
    // FSCTRL1 - IF frequency
    0x0B, 0x06,
    // FREQ2/1/0 - 868.3 MHz
    0x0D, 0x21,
    0x0E, 0x6B,
    0x0F, 0x53,
    // MDMCFG4 - channel bandwidth and data rate exponent (~4.8 kbps)
    0x10, 0x67,
    // MDMCFG3 - data rate mantissa
    0x11, 0x83,
    // MDMCFG2 - 2-FSK, no sync word (async mode handles framing)
    0x12, 0x04,
    // MDMCFG1 - preamble, channel spacing
    0x13, 0x22,
    // DEVIATN - ~4.76 kHz deviation
    0x15, 0x13,
    // MCSM0 - auto calibrate on idle to TX
    0x18, 0x18,
    // FOCCFG
    0x19, 0x16,
    // AGCCTRL2
    0x1B, 0x43,
    // AGCCTRL1
    0x1C, 0x40,
    // AGCCTRL0
    0x1D, 0x91,
    // WORCTRL
    0x20, 0xFB,
    // FSCAL3
    0x23, 0xE9,
    // FSCAL2
    0x24, 0x2A,
    // FSCAL1
    0x25, 0x00,
    // FSCAL0
    0x26, 0x1F,
    // TEST2
    0x2C, 0x81,
    // TEST1
    0x2D, 0x35,
    // TEST0
    0x2E, 0x09,
    // End marker
    0x00, 0x00,
    // PA table (max power)
    0x00, 0xC0,
};

void hormann_bisecur_send(
    uint32_t serial,
    const uint8_t* aes_key,
    uint32_t counter,
    HormannCmd cmd) {
    HormannTxState tx;
    memset(&tx, 0, sizeof(tx));
    hormann_bisecur_build_frame(tx.frame, serial, aes_key, counter, cmd);
    tx.state = TxStatePreamble;
    tx.repeat = 0;
    tx.bit_idx = 0;

    subghz_devices_init();
    const SubGhzDevice* device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
    subghz_devices_begin(device);
    subghz_devices_reset(device);
    subghz_devices_load_preset(
        device, FuriHalSubGhzPresetCustom, (uint8_t*)hormann_cc1101_regs);
    subghz_devices_set_frequency(device, HORMANN_FREQ);

    subghz_devices_set_tx(device);
    subghz_devices_start_async_tx(device, hormann_tx_callback, &tx);

    while(!subghz_devices_is_async_complete_tx(device)) {
        furi_delay_ms(10);
    }

    subghz_devices_stop_async_tx(device);
    subghz_devices_idle(device);
    subghz_devices_end(device);
    subghz_devices_deinit();
}
