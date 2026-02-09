# Hormann BiSecur Controller for Flipper Zero

Control a Hormann garage door equipped with a BiSecur receiver directly from your Flipper Zero. Compatible with HSE4-868-BS remotes. Single-door app with Open/Close/Stop/Light controls, AES-128 encrypted frames, and a persistent rolling counter.

> **Note**: Protocol constants (sync word, command codes, payload format) are best-guess placeholders based on the SX1209 packet engine. They need to be refined using captured signals from an actual HSE4-868-BS remote — see [Analyzing Saved Signals](#analyzing-saved-signals) below.

## Features

- Open / Close / Stop commands via hardware buttons
- Light toggle via long-press OK
- AES-128 encrypted frames
- Rolling counter persisted on SD card across reboots
- Pairing-based — registers as a new remote alongside your existing one
- Launches straight to control screen (no menus)

## Installation

### Prerequisites

- [uv](https://docs.astral.sh/uv/) package manager
- Flipper Zero connected via USB

### Build & Deploy

```bash
uv run ufbt          # Build -> dist/hormann_bisecur.fap
uv run ufbt launch   # Deploy and launch on Flipper
```

The app appears under **Sub-GHz > Hormann BiSecur** on the Flipper.

## Pairing

The Flipper registers as a new remote. Your existing HSE4-868-BS remote continues to work.

1. Launch the app — on first run it prompts for a door name
2. The app generates a random serial number and AES key
3. On the **Hormann receiver unit**, press and hold the **P button** (learn/program button) until the LED indicates learn mode
4. On the Flipper, press any command button (e.g. **OK** for Stop) to transmit
5. The receiver learns the new remote — pairing is complete

## Controls

| Button | Action |
|--------|--------|
| UP | Open door |
| DOWN | Close door |
| OK | Stop |
| Long OK | Toggle light |
| Back | Exit app |

## Analyzing Saved Signals

If you have an HSE4-868-BS remote, capture its signals to verify and refine the protocol constants used in this app.

### Step 1: Capture raw signals

On the Flipper Zero:

1. Go to **Sub-GHz** > **Read RAW**
2. Set frequency to **868.3 MHz** (868300000 Hz)
3. Set modulation to **FM238** (2-FSK Dev 2.38kHz) — closest available preset
4. Press the record button, then press a button on your HSE4-868-BS remote
5. Stop recording and save the `.sub` file
6. Repeat for each of the 4 buttons on the remote

### Step 2: Copy .sub files to your computer

Connect the Flipper via USB (or use qFlipper / Flipper Lab) and copy the saved `.sub` files from:

```
/ext/subghz/saved/
```

### Step 3: Inspect the raw bitstream

Each `.sub` file contains `RAW_Data` lines with alternating positive (high) and negative (low) durations in microseconds. To decode the bitstream:

```bash
# Print the raw durations from a .sub file
grep "RAW_Data" your_capture.sub
```

Look for these patterns in the timing data:

| What to find | Expected pattern |
|---|---|
| **Bit duration** | ~208 us per bit (4.8 kbps). Look for clusters of similar-duration pulses |
| **Preamble** | Alternating high/low of equal duration (0xAA = 10101010 pattern) |
| **Sync word** | A distinctive non-repeating pattern after the preamble |
| **Frame length** | Count bits between sync word and inter-frame gap |
| **Inter-frame gap** | Long low period (~20 ms) between repeated transmissions |
| **Repeat count** | How many times the frame repeats per button press |

### Step 4: Decode the frame structure

Once you have the bit timing, convert the raw durations to a bitstream:

```python
# Quick script to convert RAW_Data durations to bits
import sys

# Paste RAW_Data values here (space-separated integers)
durations = [...]  # e.g. [208, -208, 416, -208, ...]
bit_duration = 208  # us, adjust based on observed timing

bits = []
for d in durations:
    level = 1 if d > 0 else 0
    count = round(abs(d) / bit_duration)
    bits.extend([level] * count)

# Print as hex bytes
for i in range(0, len(bits), 8):
    byte = 0
    for j in range(8):
        if i + j < len(bits):
            byte |= bits[i + j] << (7 - j)
    print(f"{byte:02X}", end=" ")
print()
```

### Step 5: What to look for

Compare captures from different buttons and multiple presses of the same button:

- **Bytes that change between presses of the same button** — likely the rolling counter
- **Bytes that differ between buttons** — likely the command code
- **Bytes that stay constant** — likely the serial number (in the encrypted payload, these won't be directly visible)
- **Preamble + sync word** — these are sent in the clear (before the encrypted payload)
- **CRC** — last 2 bytes of the frame, verify CRC-16 CCITT over the payload

### Step 6: Update protocol constants

Once you've identified the frame structure, update the placeholders in `hormann_bisecur_protocol.c`:

```c
#define HORMANN_PREAMBLE_LEN   4    // Adjust based on observed preamble length
#define HORMANN_SYNC_WORD_HI   0x2D // Replace with actual sync word byte 1
#define HORMANN_SYNC_WORD_LO   0xD4 // Replace with actual sync word byte 2
#define HORMANN_BIT_DURATION   208  // Adjust based on measured bit timing
#define HORMANN_REPEAT_COUNT   3    // Adjust based on observed repetitions
```

And the command codes in `hormann_bisecur_protocol.h`:

```c
typedef enum {
    HormannCmdOpen  = 0x01,  // Replace with actual command byte
    HormannCmdClose = 0x02,
    HormannCmdStop  = 0x03,
    HormannCmdLight = 0x04,
} HormannCmd;
```

## How It Works

The app implements the Hormann BiSecur protocol:

- **Frequency**: 868.3 MHz via CC1101 with custom register preset
- **Modulation**: 2-FSK, ~4.8 kbps data rate, ~5 kHz deviation
- **Frame format**: 4-byte preamble + 2-byte sync word + 16-byte AES-128 ECB encrypted payload + 2-byte CRC-16
- **Encryption**: AES-128 ECB via STM32 hardware crypto engine
- **TX sequence**: NRZ bitstream, frame repeated 3 times with 20 ms gaps

Configuration is stored at `/ext/apps_data/hormann_bisecur/config.txt` on the SD card.

## Project Structure

```
├── application.fam                       # App manifest
├── hormann_bisecur.c/h                   # Entry point, app lifecycle
├── hormann_bisecur_protocol.c/h          # BiSecur frame encoding & radio TX
├── hormann_bisecur_store.c/h             # Door config persistence
├── images/
│   └── hormann_10px.png                  # App icon
└── scenes/
    ├── hormann_bisecur_scene.h           # Scene declarations
    ├── hormann_bisecur_scene_control.c   # Control screen
    └── hormann_bisecur_scene_add.c       # First-run door setup
```
