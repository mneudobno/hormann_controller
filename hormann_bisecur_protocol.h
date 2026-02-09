#pragma once

#include <stdint.h>

typedef enum {
    HormannCmdOpen = 0x01,
    HormannCmdClose = 0x02,
    HormannCmdStop = 0x03,
    HormannCmdLight = 0x04,
} HormannCmd;

void hormann_bisecur_send(
    uint32_t serial,
    const uint8_t* aes_key,
    uint32_t counter,
    HormannCmd cmd);
