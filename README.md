# 基于STM32+ESP32+云端大模型的嵌入式 AI 虚拟角色系统

这是一个运行在 `STM32F103VET6` 平台上的嵌入式 AI 虚拟角色项目。系统以 STM32 作为前端交互核心，结合 ESP32 的联网与音频播放能力，以及 FastAPI + 大语言模型 + TTS 的云端服务，实现了一个支持场景选择、多轮对话、语音回复和情绪联动的 AI 角色系统。

当前版本已经完成以下主链路：

- STM32 端封面页、场景选择页、对话页、情绪页
- 按键和触摸双输入方式
- STM32 与 ESP32 串口通信
- ESP32 请求 FastAPI 后端并转发回复
- LLM 动态生成回复、候选选项和情绪标签
- TTS 生成语音并由 ESP32 播放
- 本地兜底逻辑，后端异常时仍可演示

---

## 1. 项目结构

```text
STM32_AI_Character/
├─ μVision_AI_character/                  # STM32 固件工程
│  ├─ Project/RVMDK（uv5）/BH-F103.uvprojx
│  ├─ User/
│  │  ├─ main.c                           # 启动入口与硬件初始化
│  │  ├─ ai_app.c / ai_app.h              # 页面状态机、输入处理、待机逻辑
│  │  ├─ ai_ui.c / ai_ui.h                # 封面/场景/对话/情绪页绘制
│  │  ├─ ai_chat.c / ai_chat.h            # 串口协议、回包解析、本地兜底
│  │  ├─ ai_app_data.c / ai_app_data.h    # 场景数据、文案、颜色与布局常量
│  │  ├─ ai_app_utils.c / ai_app_utils.h  # 文本处理、串口辅助函数
│  │  ├─ emotion.c / emotion.h            # 情绪系统
│  │  └─ ai_avatars_new.c / ai_avatars_new.h
│  └─ Libraries/                          # CMSIS 与固件库
├─ esp32_bridge/
│  └─ STM32_AI_Bridge/
│     └─ STM32_AI_Bridge.ino              # ESP32 串口桥接与语音播放
├─ audio/                                 # 后端生成的音频文件目录
├─ docs/                                  # 需求与文档资料
├─ facial/                                # 头像与表情素材及生成脚本
├─ main.py                                # FastAPI 后端主程序
├─ manbo_speech.py                        # Fish Audio 语音生成
├─ text_to_speech.py                      # 备用/历史语音脚本
├─ ai_partner_memory.db                   # 本地数据库
├─ requirements.txt                       # Python 依赖
└─ .env.example                           # 环境变量模板
```

---

## 2. 系统架构

当前系统采用三层结构：

1. `STM32`
   负责屏幕显示、按键/触摸交互、页面切换、选项选择和本地兜底显示。

2. `ESP32`
   负责串口桥接、Wi-Fi 联网、访问后端接口、接收语音资源并通过 I2S 播放。

3. `FastAPI + LLM + TTS`
   负责生成场景化回复、三条用户选项、情绪标签和语音资源。

数据流如下：

```text
用户输入 -> STM32 -> ESP32 -> FastAPI -> LLM/TTS
                                  |
                                  v
                           回复/选项/情绪/语音
                                  |
                                  v
                         ESP32 回传 STM32 并播放语音
```

---

## 3. STM32 端功能

### 3.1 页面流程

当前主流程为：

1. 封面页
2. 场景选择页
3. 对话页
4. 情绪页

### 3.2 输入方式

- `K1 短按`：切换场景或切换当前选项
- `K1 长按`：打开/关闭情绪页
- `K2 短按`：确认场景或确认选项
- `K2 长按`：返回上一级
- 触摸点击：选择场景、点击选项、调整音量
- 手势滑动：辅助切换或触发交互事件

### 3.3 当前内置场景

当前工程内置 12 个场景：

- 逛街约会
- 一起开黑
- 咖啡馆
- 图书馆
- 晚安夜聊
- 鼓励模式
- 散步吹风
- 一起吃饭
- 电影时间
- 音乐分享
- 考前陪伴
- 休息陪伴

### 3.4 情绪系统

情绪模块位于 [emotion.c](D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/μVision_AI_character/User/emotion.c)，使用基于 `Pleasure-Arousal` 的轻量情绪模型，支持：

- 点击、长按、滑动等事件注入
- 待机时情绪自然衰减
- 映射到不同表情状态
- 在情绪页显示当前状态和音量

### 3.5 本地兜底

如果 ESP32 未连接，或后端未返回有效数据，STM32 会自动退回本地 mock 回复，不会卡死在等待状态。这一机制对现场演示很重要。

---

## 4. ESP32 串口桥接

桥接程序位于：

[STM32_AI_Bridge.ino](D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino)

### 4.1 主要职责

- 接收 STM32 串口请求
- 解析场景与选项索引
- 调用 FastAPI `/scene_story_serial`
- 将回复、选项、情绪转发给 STM32
- 播放语音音频
- 接收 STM32 发来的音量调整命令

### 4.2 当前协议

STM32 -> ESP32：

```text
SCENE:shopping
SCENE:gaming|IDX:1
SET_VOL:70
```

ESP32 -> STM32：

```text
TEXT=...|OPT1=...|OPT2=...|OPT3=...|AVATAR=happy|AUDIO=http://...
```

### 4.3 当前默认硬件配置

UART2：

- `GPIO16` -> RX
- `GPIO17` -> TX
- `115200` 波特率

I2S 音频输出：

- `BCLK = 26`
- `LRC = 25`
- `DOUT = 22`

### 4.4 烧录前必须修改

在 `.ino` 中至少确认这几项：

```cpp
static const char *WIFI_SSID = "test";
static const char *WIFI_PASS = "12345678";
static const char *API_BASE_URL = "http://192.168.24.6:8000";
static const char *API_PATH = "/scene_story_serial";
```

其中：

- `WIFI_SSID` 和 `WIFI_PASS` 需要改成你当前 Wi-Fi
- `API_BASE_URL` 需要改成运行 FastAPI 的电脑当前局域网 IP

---

## 5. 后端说明

后端入口文件：

[main.py](D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/main.py)

### 5.1 当前后端能力

- 场景化对话生成
- 多轮上下文对话
- 候选选项生成
- GBK 十六进制编码输出
- 语音生成与音频 URL 返回
- SQLite 本地用户记忆与画像存储

### 5.2 主要接口

- `GET /`
- `GET /health`
- `GET /memories`
- `GET /profile`
- `POST /scene`
- `POST /scene_serial`
- `POST /scene_story_serial`
- `POST /chat`
- `POST /reset`

其中，ESP32 当前主要访问：

- `POST /scene_story_serial`

### 5.3 环境变量

请先复制：

```text
.env.example -> .env
```

然后至少配置：

```text
DEEPSEEK_API_KEY=你的密钥
FISH_AUDIO_API_KEY=你的密钥
```

### 5.4 启动方式

在仓库根目录运行：

```bash
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 8000
```

本机测试：

```text
http://127.0.0.1:8000/docs
```

局域网测试：

```text
http://你的电脑IP:8000/docs
```

如果手机打不开局域网地址，ESP32 通常也无法连接后端。

---

## 6. 语音系统说明

当前语音逻辑使用：

[manbo_speech.py](D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/manbo_speech.py)

主要流程如下：

1. 后端根据 AI 回复文本和情绪标签调用 TTS
2. 语音文件生成到 `audio/` 目录
3. FastAPI 通过 `/audio` 暴露静态资源
4. ESP32 获取 `audio_url`
5. ESP32 通过 I2S + 功放模块播放

当前按情绪映射不同参考音色模型，例如：

- `happy`
- `shy`
- `gentle`
- `curious`
- `thinking`

---

## 7. 编译与运行

### 7.1 STM32

打开工程：

[BH-F103.uvprojx](D:/2025-2026-2/FPGA/CubeIDE/STM32_AI_Character/μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx)

环境建议：

- `Keil MDK-ARM / μVision5`
- `ARM Compiler 5`
- 已验证版本：`V5.06 update 6 (build 750)`

编译步骤：

1. 打开 `BH-F103.uvprojx`
2. 选择 `Target: LDC`
3. 点击 `Rebuild`
4. 下载到开发板

### 7.2 ESP32

使用 Arduino IDE 或 PlatformIO：

1. 打开 `STM32_AI_Bridge.ino`
2. 修改 Wi-Fi 和 `API_BASE_URL`
3. 确认音频模块接线
4. 烧录 ESP32
5. 打开串口监视器，波特率 `115200`

正常日志应类似：

```text
[bridge] boot
[bridge] uart2 baud=115200
[bridge] connecting wifi...
[bridge] wifi ok, ip=...
```

### 7.3 联调顺序

推荐顺序：

1. 先编译并运行 STM32，确认本地 UI 正常
2. 启动 FastAPI 后端
3. 用手机测试 `http://电脑IP:8000/docs`
4. 烧录并启动 ESP32
5. 最后连接 STM32 和 ESP32 联调

---

## 8. 常见问题

### 8.1 STM32 只显示默认回复

常见原因：

- ESP32 没有成功访问后端
- 串口回包不完整
- 回包解析失败
- 网络异常时系统进入本地兜底

优先检查：

- 串口监视器是否有 `HTTP error`
- `API_BASE_URL` 是否正确
- 手机能否访问 `http://电脑IP:8000/docs`

### 8.2 ESP32 串口显示 `HTTP error:-1`

这通常表示：

- 后端没启动
- `API_BASE_URL` 地址错误
- 电脑防火墙拦住了 `8000`
- ESP32 和电脑不在同一局域网

### 8.3 中文乱码

当前工程采用 GBK 兼容策略：

- 后端返回 `reply_gbk_hex`
- ESP32 将十六进制还原成字节流
- STM32 按协议解析显示

如果又出现乱码，优先检查：

- 后端是否仍在返回 `reply_gbk_hex` / `options_gbk_hex`
- ESP32 是否仍使用 `buildSerialPacketFromHex`
- STM32 是否错误回退到旧的解析逻辑

---


---

## 9. 后续扩展方向

当前架构已经适合继续扩展：

- 视觉感知模块
- 好感度与长期记忆
- 更丰富的剧情分支
- 更完整的角色人格系统
- 更细腻的 UI 动画与情绪演出




