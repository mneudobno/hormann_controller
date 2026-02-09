# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Deploy

```bash
uv run ufbt          # Build -> produces dist/hormann_bisecur.fap
uv run ufbt launch   # Build, deploy to connected Flipper, and launch
```

Uses `uv` to manage Python/ufbt. The Flipper SDK lives at `~/.ufbt/current/`.

## Architecture

Flipper Zero app implementing the Hormann BiSecur radio protocol for controlling garage doors equipped with BiSecur receivers (HSE4-868-BS compatible). Written in C against the Flipper firmware SDK.

### BiSecur Protocol

- **Frequency**: 868.3 MHz
- **Modulation**: 2-FSK, ~4.8 kbps data rate, ~5 kHz deviation
- **Frame**: Preamble + Sync word + 16-byte AES-128 ECB encrypted payload + CRC-16
- **Encryption**: AES-128 ECB via mbedTLS
- **Pairing**: User puts receiver in learn mode (P button), then sends command from Flipper

### Scene-based navigation

The app uses Flipper's `SceneManager` + `ViewDispatcher` pattern. Four scenes, each with `on_enter`/`on_event`/`on_exit` callbacks:

- **Menu** (`scenes/hormann_bisecur_scene_menu.c`) -- Submenu listing doors. Short-press selects, long-press deletes. "Add Door" item at bottom.
- **Control** (`scenes/hormann_bisecur_scene_control.c`) -- Raw `View` with custom draw/input callbacks for direct key handling. UP=Open, OK=Stop, DOWN=Close, Hold OK=Light.
- **Add** (`scenes/hormann_bisecur_scene_add.c`) -- TextInput for door name. Generates random 32-bit serial and 128-bit AES key.
- **Confirm Delete** (`scenes/hormann_bisecur_scene_confirm_delete.c`) -- DialogEx confirmation.

### Protocol layer (`hormann_bisecur_protocol.c`)

BiSecur 2-FSK frame encoding and async TX via CC1101 with custom register preset. NRZ bitstream at ~208us per bit. State machine: Preamble -> SyncWord -> Data -> CRC -> Gap, repeated 3x.

### Storage (`hormann_bisecur_store.c`)

Door configs persisted to `APP_DATA_PATH("config.txt")` using FlipperFormat. Stores name, serial, AES key (hex), and rolling counter. Counter saved after every transmission.

### Key constants

- `HORMANN_MAX_DOORS (8)` -- fixed array, no dynamic allocation
- `HORMANN_AES_KEY_SIZE (16)` -- 128-bit AES key per door

### Protocol unknowns (placeholders)

Command codes, sync word, and exact payload format are best-guess based on SX1209 defaults. They should be refined using captured signals from an HSE4-868-BS remote.
