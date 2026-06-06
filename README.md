# STM32 AI Virtual Character

Embedded AI virtual character project based on the `STM32F103VET6` Wildfire development board.

This repository contains three parts:

- `STM32`: cover page, scene selection page, chat page, key/touch interaction, avatar and text rendering
- `ESP32`: serial bridge between `STM32 -> ESP32 -> FastAPI`
- `FastAPI + LLM`: dynamic reply, option generation, avatar mood selection

The current repository is organized so it can be opened and compiled directly in `Keil uVision`.

## Project Structure

- `μVision_AI_character/`
  STM32 firmware project
- `μVision_AI_character/User/main.c`
  Minimal entry file, only `main()` and hardware startup
- `μVision_AI_character/User/ai_app.c`
  Page state machine, input dispatch, idle logic
- `μVision_AI_character/User/ai_ui.c`
  UI drawing, avatar rendering, text and option display
- `μVision_AI_character/User/ai_chat.c`
  Serial protocol, reply parsing, local mock fallback
- `μVision_AI_character/User/ai_app_data.c`
  Scene table, UI text, layout constants
- `μVision_AI_character/User/ai_app_utils.c`
  Text wrapping, ellipsis trimming, string helpers, UART helpers
- `esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`
  ESP32 serial bridge
- `main.py`
  FastAPI backend entry
- `docs/v3_modules_requirements.md`
  Requirement notes for planned modules

## STM32 Build

### Environment

- `Keil MDK-ARM / uVision5`
- `ARM Compiler 5`

Verified compiler version:

- `V5.06 update 6 (build 750)`

### Open the Project

Open this file in `uVision`:

`μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx`

Target information:

- Target: `LDC`
- Device: `STM32F103VE`
- Output: `Template`

### Build Steps

1. Open `BH-F103.uvprojx`
2. Confirm the active target is `LDC`
3. Click `Rebuild`

The project file already includes the split modules in the `USER` group. No manual project-file editing is required.

## STM32 Runtime Flow

After reset, the firmware enters:

1. Cover page
2. Scene selection page
3. Multi-round chat page

If ESP32 or backend is not connected, the firmware falls back to local mock replies so the UI demo still works.

## STM32 Module Notes

### `main.c`

Only keeps:

- `main()`
- `System_Init_All()`

### `ai_app.c`

Responsible for:

- page state machine
- `K1 / K2` input logic
- touch and gesture dispatch
- scene and option switching
- idle feedback logic

### `ai_ui.c`

Responsible for:

- cover page drawing
- scene page drawing
- chat page drawing
- avatar scaling and drawing
- dialog and option rendering

### `ai_chat.c`

Responsible for:

- first-round scene request
- option sending
- serial reply parsing
- mock reply building
- avatar token parsing

### `ai_app_data.c`

Responsible for:

- scene metadata
- UI copy
- idle feedback strings
- layout constants

### `ai_app_utils.c`

Responsible for:

- pixel-width-based text wrapping
- ellipsis trimming
- string helpers
- UART helpers

## Interaction

- `K1 short`: next option or next scene
- `K1 long`: previous option or previous scene
- `K2 short`: confirm selection
- `K2 long`: go back
- touch tap: select scene or option
- touch swipe: scene or page navigation

## Example Scenes

Built-in scenes include:

- Shopping date
- Gaming
- Cafe
- Library
- Night chat
- Encourage mode
- Walk
- Dinner
- Movie
- Music
- Exam
- Rest

## ESP32 Bridge

Bridge file:

`esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`

### Protocol

STM32 -> ESP32:

```text
SCENE:shopping
SCENE:shopping|IDX:0
SCENE:gaming|IDX:2
```

ESP32 -> STM32:

```text
TEXT=...|OPT1=...|OPT2=...|OPT3=...|AVATAR=happy
```

### Required ESP32 Config

Check these values before flashing:

- `WIFI_SSID`
- `WIFI_PASS`
- `API_BASE_URL`
- `API_PATH`

### Recommended Wiring

Current STM32 firmware uses `USART2`:

- `STM32 PA2 = TX`
- `STM32 PA3 = RX`

ESP32 bridge default:

- `GPIO16 = RX`
- `GPIO17 = TX`

Wire as:

- `ESP32 GPIO17 (TX) -> STM32 PA3 (RX)`
- `ESP32 GPIO16 (RX) -> STM32 PA2 (TX)`
- `GND -> GND`

### ESP32 Serial Monitor

Baud rate:

```text
115200
```

Expected boot log:

```text
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://your-pc-ip:8000/scene_story_serial
[bridge] connecting wifi...
[bridge] wifi ok, ip=...
```

## FastAPI Backend

Backend entry:

`main.py`

### Python Environment

Recommended:

- `Python 3.10+`

### Dependencies

If `requirements.txt` is not yet present, install manually:

```bash
pip install fastapi uvicorn python-dotenv openai pydantic
```

### Environment Variables

Copy:

```text
.env.example -> .env
```

Then set:

```text
DEEPSEEK_API_KEY=your_key
```

### Start Backend

Run in repository root:

```bash
uvicorn main:app --host 0.0.0.0 --port 8000
```

Then open:

`http://127.0.0.1:8000/docs`

### Main API

- `POST /scene_serial`
- `POST /scene_story_serial`

Recommended real integration endpoint:

- `POST /scene_story_serial`

## Quick Test Paths

### A. STM32 Only

1. Open project in `uVision`
2. Build and flash STM32
3. Verify cover, scene, and chat pages

### B. STM32 + ESP32

1. Build and flash STM32
2. Configure `STM32_AI_Bridge.ino`
3. Flash ESP32
4. Connect serial wires
5. Watch ESP32 serial log

### C. Full Chain

1. Start FastAPI
2. Flash ESP32
3. Flash STM32
4. Enter a scene
5. Verify dynamic reply and options

## Common Issues

### uVision builds but flashing fails

Check:

- debugger connection
- `ST-Link`
- `Connect under Reset`
- board power

### ESP32 shows `http failed, status=-1`

Usually backend address or local network issue. Check:

- `API_BASE_URL`
- actual PC LAN IP
- backend started with `0.0.0.0`
- firewall allows port `8000`

### STM32 screen and ESP32 log do not match

Check:

- crossed TX/RX wiring
- current firmware is the `USART2` version
- STM32 is flashed with the latest build

### Chinese text becomes garbled

This firmware path depends on GBK-compatible rendering on STM32. Check:

- STM32 is updated to latest build
- ESP32 bridge matches current repository version
- serial reply is sent as the expected packet format

## Current Status

This repository is currently a demo-ready prototype, not a final packaged release.

Already implemented:

- STM32 modular UI and interaction flow
- ESP32 serial bridge
- FastAPI + LLM dynamic dialogue
- scene-based multi-round replies

Planned expansion:

- voice module
- affection and mood memory
- vision sensing module
- story branch module

