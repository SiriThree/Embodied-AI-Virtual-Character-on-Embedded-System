# ESP32 Bridge

这个示例把 `STM32 -> ESP32 -> FastAPI` 这条链路接通，让开场白和每一轮三选项都由后端 LLM 动态生成。

## 串口协议

STM32 发给 ESP32：

```text
SCENE:shopping
SCENE:shopping|USER:你会不会偷偷夸我
```

ESP32 回给 STM32：

```text
TEXT=...|OPT1=...|OPT2=...|OPT3=...|AVATAR=happy
```

## 使用方式

1. 在 [STM32_AI_Bridge.ino](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/esp32_bridge/STM32_AI_Bridge.ino) 里填写：
   - `WIFI_SSID`
   - `WIFI_PASS`
   - `API_BASE_URL`
2. 确保 FastAPI 后端已经启动，并且能访问：
   - `POST /scene_story_serial`
3. 根据你的接线调整：
   - `STM32_RX_PIN`
   - `STM32_TX_PIN`
   - `STM32_BAUD`
4. 在 Arduino IDE 或 PlatformIO 编译并烧录 ESP32。

## 设计说明

- ESP32 会按场景保存最近几轮 `history`，所以每次用户选完后，下一轮会把上下文继续带给 LLM。
- STM32 现在建议通过 `|IDX:0/1/2` 回传选中的选项编号，ESP32 会用上一次后端返回的 UTF-8 选项文本恢复真实历史，避免中文编码导致 `HTTP 400`。
- 第一次进入场景时，`SCENE:xxx` 会清空该场景历史，所以开场白也会重新生成，不再固定。
- 后端接口默认走更偏 galgame 风格的 `/scene_story_serial`。
- 如果 Wi-Fi 或后端暂时失败，ESP32 会回一条简短兜底串，STM32 不会卡死。
