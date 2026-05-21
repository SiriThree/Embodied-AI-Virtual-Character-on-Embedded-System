# XPT2046 touch driver for AI project

本包基于官方“电阻触摸屏—触摸画板”工程中的 XPT2046 驱动整理。

已修改内容：
1. 移除了触摸画板专用的 `palette.h` 依赖。
2. 移除了 `bsp_led.h` 依赖。
3. 清空了 `XPT2046_TouchDown()` 和 `XPT2046_TouchUp()` 中的画板绘图逻辑。
4. 保留 `XPT2046_Init()`、`Calibrate_or_Get_TouchParaWithFlash()`、`XPT2046_Get_TouchedPoint()` 等触摸采集核心函数。
5. 在头文件中补充声明 `XPT2046_TouchDetect()`。

使用方式：
- 将 `bsp_xpt2046_lcd.c/.h` 放入当前 AI 工程的 `User/lcd/`。
- 在 Keil 中 Add `bsp_xpt2046_lcd.c` 到工程。
- main.c 中 include：`#include "./lcd/bsp_xpt2046_lcd.h"`
- 初始化顺序建议：
  ```c
  ILI9341_Init();
  USART_Config();
  ILI9341_GramScan(6);
  XPT2046_Init();
  Calibrate_or_Get_TouchParaWithFlash(6, 0);
  AI_UI_Init();
  ```

注意：
- 不要在 AI 工程中调用 `XPT2046_TouchEvenHandler()` 做画板逻辑。
- 后续手势识别建议直接使用 `XPT2046_PENIRQ_Read()` + `XPT2046_Get_TouchedPoint()` 采集触摸轨迹。
