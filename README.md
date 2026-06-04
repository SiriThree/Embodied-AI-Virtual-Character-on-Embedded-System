# STM32 AI Virtual Character

基于 `STM32F103VET6` 野火开发板的嵌入式 AI 虚拟角色系统。项目包含三部分：

- `STM32` 端：封面页、场景选择页、对话页、按键/触摸交互、头像与文本显示
- `ESP32` 端：串口桥接 `STM32 -> ESP32 -> FastAPI`
- `FastAPI + LLM` 后端：根据场景和上下文动态生成 AI 回复、选项和表情



## 目录结构

- `μVision_AI_character/`
  STM32 工程和全部板级代码
- `esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`
  ESP32 串口桥接程序
- `main.py`
  FastAPI 后端入口
- `ai_partner_memory.db`
  本地记忆数据库

## 1. STM32 直接编译说明

### 1.1 开发环境

- `Keil MDK-ARM / μVision5`
- 已安装 `ARM Compiler 5`
  本项目当前验证环境为 `V5.06 update 6 (build 750)`

### 1.2 工程位置

在 `μVision` 中打开：

[`μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx)

工程关键信息：

- Target：`LDC`
- Device：`STM32F103VE`
- Output：`Template`

### 1.3 工程已包含的内容

默认已经包含：

- `CMSIS`
- `FWlib`
- LCD 显示驱动
- 触摸驱动
- 按键驱动
- 手势模块
- 当前 UI 主逻辑

不需要手动再补 `Include Path` 或额外新建工程。

### 1.4 直接编译步骤

1. 打开 `BH-F103.uvprojx`
2. 确认左上角 Target 是 `LDC`
3. 点击 `Rebuild`

如果环境正常，应该可以直接通过编译。

### 1.5 下载与运行

STM32 固件下载完成后，`RESET` 上电应进入：

1. 封面页
2. 场景选择页
3. 对话页

如果未接 ESP32/后端，系统会走本地兜底回复，不会卡死。

## 2. STM32 端当前功能

### 2.1 页面流程

- 封面页
- 场景选择页
- 多轮对话页

### 2.2 交互方式

- `K1`
  切换选项
- `K2`
  确认选项
- 长按 `K2`
  返回场景页
- 触摸点击
  选择场景或选项
- 触摸滑动
  场景翻动或选项切换

### 2.3 场景示例

当前内置多个场景，包括：

- 逛街约会
- 一起开黑
- 图书馆
- 晚安夜聊
- 鼓励模式
- 音乐分享
- 电影时间

### 2.4 UI 显示

- 顶部：场景标题
- 上半屏：AI 头像
- 中部：用户文本与 AI 回复
- 下部：3 条可选回应

## 3. 如果只想先做 STM32 演示

不接 ESP32 与后端时，也可以演示：

- 页面切换
- 场景选择
- 多选项对话
- 头像/文本/触摸交互

这是最适合先验收硬件显示和交互链路的方式。

## 4. ESP32 联调说明

ESP32 bridge 文件：

[`esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino)

### 4.1 作用

负责把 STM32 发来的串口帧转发给后端，再把后端生成结果回给 STM32。

### 4.2 当前串口协议

STM32 -> ESP32

```text
SCENE:shopping
SCENE:shopping|IDX:0
SCENE:gaming|IDX:2
```

ESP32 -> STM32

```text
TEXT=...|OPT1=...|OPT2=...|OPT3=...|AVATAR=happy
```

### 4.3 ESP32 需要修改的配置

烧录前至少确认：

- `WIFI_SSID`
- `WIFI_PASS`
- `API_BASE_URL`
- `API_PATH`

### 4.4 推荐接线

当前 STM32 工程已切到 `USART2`：

- `STM32 PA2 = TX`
- `STM32 PA3 = RX`

ESP32 bridge 默认：

- `GPIO16 = RX`
- `GPIO17 = TX`

连接方式：

- `ESP32 GPIO17 (TX)` -> `STM32 PA3 (RX)`
- `ESP32 GPIO16 (RX)` -> `STM32 PA2 (TX)`
- `GND -> GND`

### 4.5 ESP32 串口监视器

波特率：

```text
115200
```

正常启动时应看到类似：

```text
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://你的电脑IP:8000/scene_story_serial
[bridge] connecting wifi...
[bridge] wifi ok, ip=...
```

## 5. FastAPI 后端说明

后端入口：

[`main.py`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/main.py)

### 5.1 Python 环境

建议：

- `Python 3.10+`

### 5.2 依赖安装

仓库当前未单独提供 `requirements.txt`，可先手动安装：

```bash
pip install fastapi uvicorn python-dotenv openai pydantic
```

### 5.3 环境变量

先复制：

```text
.env.example -> .env
```

然后在 `.env` 中填写：

```text
DEEPSEEK_API_KEY=你的密钥
```

### 5.4 启动方式

在仓库根目录运行：

```bash
uvicorn main:app --host 0.0.0.0 --port 8000
```

启动后可访问：

[`http://127.0.0.1:8000/docs`](http://127.0.0.1:8000/docs)

### 5.5 当前主要接口

- `POST /scene_serial`
- `POST /scene_story_serial`

推荐实际联调用：

- `POST /scene_story_serial`

它会返回：

- AI 回复
- 3 条动态选项
- 头像情绪
- `GBK hex`
- 可直接发给 STM32 的串口格式

## 6. 一套可跑通的最短流程

### 方案 A：只验证 STM32

1. 在 `μVision` 打开工程
2. 编译并烧录 STM32
3. 上电查看封面页、场景页、对话页

### 方案 B：STM32 + ESP32

1. 编译并烧录 STM32
2. 配好 `STM32_AI_Bridge.ino`
3. 烧录 ESP32
4. 串口接线
5. 观察 ESP32 串口监视器日志

### 方案 C：完整联调

1. 启动 FastAPI
2. 烧录 ESP32
3. 烧录 STM32
4. 进入场景
5. 查看动态生成的回复和选项

## 7. 常见问题

### 7.1 μVision 能编译，但烧录后没反应

优先检查：

- 下载器连接
- `ST-Link`
- `Connect under Reset`
- 芯片供电

### 7.2 ESP32 串口监视器出现 `http failed, status=-1`

通常是后端地址或网络问题，检查：

- `API_BASE_URL`
- 电脑实际局域网 IP
- `uvicorn` 是否用 `0.0.0.0`
- Windows 防火墙是否放行 `8000`

### 7.3 STM32 和 ESP32 日志不一致

优先检查：

- 串口接线是否交叉
- 是否使用当前 README 中的 `USART2`
- STM32 是否已烧录到最新固件

### 7.4 屏幕中文乱码

当前工程对 STM32 侧中文显示依赖 `GBK` 字节兼容路径。若出现乱码，优先检查：

- STM32 是否烧录到最新版本
- ESP32 bridge 是否为当前仓库版本
- 串口回包是否走 `GBK packet`

## 8. 当前状态说明

这个仓库当前更偏“可演示原型”，不是最终封装完成版。  
已经具备：

- STM32 本地 UI 与交互
- ESP32 串口桥接
- FastAPI + LLM 动态对话
- 基础场景化多轮回复

后续仍可继续扩展：

- 声音模块
- 好感度与情绪记忆模块
- 视觉感知模块
- 剧情分支模块

## 9. 建议提交前自检

如果你准备把仓库交给别人使用，建议先确认这 5 件事：

1. `BH-F103.uvprojx` 能在本机 `μVision` 正常打开
2. `LDC` Target 能直接编译通过
3. `.env.example` 内容完整
4. `STM32_AI_Bridge.ino` 中的 Wi-Fi 和 IP 已改成示例值或注释清楚
5. README 中的工程路径与实际仓库一致

## 10. 相关文件

- [`μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx)
- [`μVision_AI_character/User/main.c`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/μVision_AI_character/User/main.c)
- [`esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino)
- [`main.py`](/D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/main.py)

