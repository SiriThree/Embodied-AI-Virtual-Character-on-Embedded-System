# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an embedded AI virtual character system that runs on STM32F103VET6 with a touch screen. The system creates an interactive AI companion with personality, interactivity, and companionship. It consists of three interconnected components:

1. **STM32 Firmware**: UI rendering, touch/button input, state management (Keil μVision project)
2. **ESP32 Bridge**: Serial bridge between STM32 and backend (Arduino/PlatformIO)
3. **FastAPI Backend**: LLM-powered dynamic conversation generation (Python)

## Architecture

### Communication Flow

```
STM32 (UI/Input) <--UART2--> ESP32 (WiFi Bridge) <--HTTP--> FastAPI Backend <--> DeepSeek LLM
```

### Serial Protocol

**STM32 → ESP32:**
- Initial scene: `SCENE:shopping`
- With user choice: `SCENE:shopping|IDX:0` (IDX is the selected option index 0/1/2)

**ESP32 → STM32:**
- Response format: `TEXT=角色回复|OPT1=选项1|OPT2=选项2|OPT3=选项3|AVATAR=happy`
- Avatar states: `happy`, `shy`, `gentle`, `thinking`, `curious`

### STM32 Module Structure

The firmware is organized into focused modules:

- **ai_app.c**: Page state machine (cover → scene selection → chat), input routing, idle feedback
- **ai_ui.c**: Rendering for all three pages, avatar display, text wrapping
- **ai_chat.c**: Serial protocol handling, response parsing, local mock fallback
- **ai_app_data.c**: Scene definitions, UI text constants, layout parameters
- **ai_app_utils.c**: Text truncation, line wrapping, serial helpers

### Backend Structure

The FastAPI backend in `main.py` provides:
- `/scene_story_serial`: Main endpoint for STM32 integration (galgame style)
- `/scene_serial`: Alternative conversation endpoint
- `/chat`: General chat endpoint
- SQLite database for conversation memory (`ai_partner_memory.db`)
- LLM integration via DeepSeek API (OpenAI-compatible)

## Build and Run Commands

### STM32 Firmware

**Environment**: Keil MDK-ARM / μVision5 with ARM Compiler 5 (tested: V5.06 update 6 build 750)

**Build:**
1. Open `μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx` in Keil μVision
2. Ensure target is set to `LDC`
3. Click "Rebuild" or press F7

**Flash**: Use ST-Link with "Connect under Reset" if needed

**Hardware**: STM32F103VET6 野火开发板 with resistive touch screen

### ESP32 Bridge

**Environment**: Arduino IDE or PlatformIO

**Configuration** (edit before flashing):
```cpp
// In esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino
const char* WIFI_SSID = "your_wifi";
const char* WIFI_PASS = "your_password";
String API_BASE_URL = "http://192.168.x.x:8000";  // Your PC's LAN IP
String API_PATH = "/scene_story_serial";
```

**Pin connections**:
- ESP32 GPIO17 (TX) → STM32 PA3 (RX)
- ESP32 GPIO16 (RX) → STM32 PA2 (TX)
- GND → GND

**Flash**: Compile and upload via Arduino IDE/PlatformIO

**Monitor**: Serial monitor at 115200 baud to see bridge logs

### FastAPI Backend

**Environment**: Python 3.10+

**Setup:**
```bash
# Install dependencies
pip install -r requirements.txt

# Configure API key
cp .env.example .env
# Edit .env and add: DEEPSEEK_API_KEY=your_key_here
```

**Run:**
```bash
# Start server (must use 0.0.0.0 for ESP32 to access from LAN)
uvicorn main:app --host 0.0.0.0 --port 8000

# Access API docs
# http://127.0.0.1:8000/docs
```

**Test endpoint:**
```bash
curl -X POST http://localhost:8000/scene_story_serial \
  -H "Content-Type: application/json" \
  -d '{"scene_name":"咖啡馆","scene_description":"安静的下午","ai_intro":"要来杯咖啡吗？","history":[]}'
```

## Development Workflows

### Testing STM32 UI Only
Flash STM32 firmware and verify cover page, scene selection, and chat page rendering without backend. The system has local mock responses to avoid blocking.

### Full System Integration
1. Start FastAPI backend with `uvicorn main:app --host 0.0.0.0 --port 8000`
2. Flash ESP32 with correct WiFi and API_BASE_URL configuration
3. Connect ESP32 and STM32 via UART (TX↔RX crossover)
4. Flash STM32 firmware
5. Enter a scene and verify dynamic AI responses

### Debugging Serial Communication
- Monitor ESP32 serial output at 115200 baud for HTTP request/response logs
- Check for `[bridge] http ok` vs `[bridge] http failed` messages
- Verify TX/RX crossover wiring if STM32 shows no response
- Confirm Windows Firewall allows port 8000 if ESP32 can't reach backend

## Key Implementation Details

### Character Encoding
- Backend generates UTF-8 responses
- ESP32 passes UTF-8 to STM32 via serial
- STM32 firmware expects GBK encoding for Chinese display
- The protocol uses `|` as delimiter, so all text is sanitized to remove `|` characters

### Scene Management
- 12 built-in scenes defined in `ai_app_data.c`: shopping, gaming, cafe, library, goodnight chat, etc.
- Each scene has: name, display name, description, intro prompt
- First request to a scene (`SCENE:xxx`) clears history and generates fresh opening
- Subsequent requests include conversation history via `SCENE:xxx|IDX:n`

### Input Handling
- **K1 short press**: Next scene/option (cycle forward)
- **K1 long press**: Previous scene/option (cycle backward)
- **K2 short press**: Confirm selection
- **K2 long press**: Return to previous page
- **Touch tap**: Direct selection on scene or option
- **Touch swipe**: Page navigation

### Idle Behavior
- After 10 seconds of no input, system shows idle hint text on cover page
- Idle hints cycle through predefined messages in `ai_app_data.c`
- Any key or touch input resets idle counter

### Local Fallback
If ESP32 is not connected or backend is unreachable, STM32 uses mock responses from `ai_chat.c` so the UI remains functional for demonstration.

## Common Issues

### STM32 displays garbled Chinese text
- Ensure latest firmware is flashed with GBK font support
- Verify ESP32 bridge matches current protocol (returns `TEXT=...` format)
- Check serial baud rate is 115200 on both sides

### ESP32 reports "http failed, status=-1"
- Verify `API_BASE_URL` points to correct LAN IP address (not 127.0.0.1)
- Ensure backend is running with `--host 0.0.0.0`
- Check Windows Firewall allows incoming connections on port 8000
- Confirm ESP32 and PC are on same WiFi network

### STM32 screen freezes or shows no response
- Check TX/RX wiring (must be crossed: TX→RX, RX→TX)
- Verify UART2 is used (PA2=TX, PA3=RX) in current firmware
- Ensure both STM32 and ESP32 use 115200 baud rate
- Check STM32 power supply and ST-Link connection

### Backend returns 400 Bad Request
- ESP32 now sends `IDX:0/1/2` instead of full UTF-8 text to avoid encoding issues
- Verify ESP32 bridge is using the index-based protocol
- Check backend logs for JSON parsing errors

## Project Status and Future Extensions

Current state: Working prototype with modular STM32 firmware, functional serial bridge, and LLM-powered dynamic conversations.

Documented future modules (see `docs/v3_modules_requirements.md`):
- Audio module for voice interaction
- Emotion and affection memory system
- Visual perception module
- Story branching system
