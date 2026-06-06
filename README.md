# AI 虚拟角色系统

一个运行在 STM32 平台上的嵌入式 AI 虚拟角色系统，配备触摸屏交互界面。
该项目融合了 情绪建模、手势交互、实时动画以及基于云端大语言模型的智能对话，旨在打造一个具有生命感和互动能力的 AI 伙伴。

## ✨ 项目特点

- **AI 虚拟角色**
  - 动态动画驱动的虚拟角色
  - 情绪驱动的交互与响应

- **情绪建模系统**
  - 多维情绪状态
  - 情绪随时间和交互过程动态演变
  - 情绪影响行为表现与动画效果

- **手势交互（TinyML）**
  - 触摸轨迹识别
  - 支持基于手势的交互方式
  - 轻量化模型运行于嵌入式硬件

- **AI 对话功能**
  - 基于云端的大语言模型（LLM）交互
  - 上下文感知式对话
  - 人格化驱动的回复逻辑


- **实时动画引擎**
  - 基于Sprite的角色动画
  - 基于情绪的动画切换机制

- **触摸屏交互界面**
  - 为嵌入式设备定制的交互式图形界面（GUI）
  - 实时反馈与视觉响应

---

## 🧠 系统架构
| Touch Screen | 
- >
| STM32 Embedded System|

| - UI Engine |

| - Animation Engine |

| - Emotion Engine |

| - Gesture Recognition|

- >
| Edge AI Service |

| - Agent Controller |

| - Memory System |

| - Personality Model |

- >
  
| LLM API |



---

## 🖥 硬件配置

- STM32F103VET6 微控制器
- 触摸屏液晶显示器

---

## 🧩 软件栈

Embedded Side:

- C / C++
- LVGL GUI Library
- TinyML inference

Edge AI Service:

- Python
- LLM API
- Emotion & behavior controller

---

## 🎮 交互示例

用户触摸虚拟角色
→ 角色呈现愉悦情绪

用户绘制指定手势
→ 手势识别触发对应动画效果

用户闲置设备未操作
→ AI 角色产生无聊情绪并主动发起对话

------

## 📊 未来优化方向

- 语音交互功能
- 设备端 TinyML 情绪识别
- 基于强化学习的行为演变机制
- 多角色交互能力

---


# 项目结构与硬件连接
# 嵌入式AI虚拟角色系统 - 项目结构与硬件连接

## 📋 项目概述

该项目是一个运行在 **STM32F103VET6** 上的嵌入式AI虚拟角色交互系统，通过 **ESP32-WRQOM-32E** 作为网络网关与云端LLM服务通信，为用户提供情感化的交互体验。

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    云端 LLM API 服务                              │
│              (http://0.0.0.0:8000/scene_story_serial)       │
└──────────────────────────────┬──────────────────────────────────┘
                               │ HTTP JSON
                               │
┌──────────────────────────────▼──────────────────────────────────┐
│                   ESP32-WRQOM-32E 网络网关                       │
│          (WiFi + UART2 串口通信 @ 115200 baud)                  │
└──────────────────────────────┬──────────────────────────────────┘
                               │ UART
                               │ (SCENE:|IDX:)
                               │
┌──────────────────────────────▼──────────────────────────────────┐
│           STM32F103VET6 主控制器 + ILI9341 LCD                   │
│  ┌─────────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │  UI/Animation   │  │ Emotion      │  │ Gesture      │        │
│  │  Engine         │  │ Engine       │  │ Recognition  │        │
│  └─────────────────┘  └──────────────┘  └──────────────┘        │
│                                                                  │
│  Display: 160x120 LCD (ILI9341)  Touch: XPT2046                 │
│  Buttons: K1, K2                                                │
└──────────────────────────────────────────────────────────────────┘
```

---

## 🔌 硬件连接详解

### 1. ESP32 ↔ STM32 串口通信

| 连接点 | ESP32 | STM32F103 | 功能 |
|---------|--------|-----------|------|
| **RX** | GPIO 16 (UART2 RX) | PA2 (UART2 TX) | 接收STM32数据 |
| **TX** | GPIO 17 (UART2 TX) | PA3 (UART2 RX) | 发送数据给STM32 |
| **GND** | GND | GND | 地线 |
| **波特率** | 115200 bps | 115200 bps | 数据率 |

**ESP32代码配置** (`esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`):
```cpp
// UART2 <-> STM32
static const int STM32_RX_PIN = 16;      // ESP32 接收脚 (来自STM32 TX)
static const int STM32_TX_PIN = 17;      // ESP32 发送脚 (到STM32 RX)
static const uint32_t STM32_BAUD = 115200;
---

### 2. STM32F103VET6 - 触摸LCD 接口

#### 📺 LCD屏幕 (ILI9341) - FSMC接口

| 信号 | STM32 引脚 | 功能 | 说明 |
|------|-----------|------|------|
| **命令** | - | 写命令 | 通过FSMC访问地址 0xA0000000 |
| **数据** | - | 写数据 | 通过FSMC访问地址 0xA0000002 |

**宏定义**:
```c
#define LCD_CMD   (*((volatile uint16_t *)FSMC_Addr_ILI9341_CMD))
#define LCD_DATA  (*((volatile uint16_t *)FSMC_Addr_ILI9341_DATA))
```

- **屏幕分辨率**: 240×320 像素
- **触摸屏**: 160×120 显示区域 (用于头像)
- **接口类型**: FSMC (灵活的静态内存控制器)

---

#### ✏️ 触摸屏 (XPT2046) - SPI接口

**引脚配置** (`xpt2046_ai_ready/bsp_xpt2046_lcd.h`):

| 引脚 | 端口 | 管脚 | 功能 |
|------|------|------|------|
| **CS** | GPIOD | Pin 13 | 芯片选择 |
| **CLK** | GPIOE | Pin 0 | 时钟 |
| **MOSI** | GPIOE | Pin 2 | 主出从入 |
| **MISO** | GPIOE | Pin 3 | 主入从出 |
| **PENIRQ** | GPIOE | Pin 4 | 触摸中断 (低有效) |

**数据通道配置**:
```c
#define XPT2046_CHANNEL_X  0x90    // Y+通道 (实际读X坐标)
#define XPT2046_CHANNEL_Y  0xd0    // X+通道 (实际读Y坐标)
```

---

### 3. STM32F103VET6 - 用户输入接口

#### 🔘 按键输入

| 按键 | 功能 | 位置 |
|------|------|------|
| **K1** | 上/切换/选择 | 左按键 |
| **K2** | 确认/进入 | 右按键 |
| **长按 K2** | 返回/退出 | 2秒+ |

**交互模式**:
- PAGE_COVER: K1/K2 进入场景选择
- PAGE_SCENE_SELECT: K1前后翻页, K2进入聊天
- PAGE_CHAT: K1选择选项, K2确认

#### ✋ 手势识别

支持的手势 (触摸屏):
- **TAP** - 单点击
- **SWIPE_LEFT** - 向左滑
- **SWIPE_RIGHT** - 向右滑
- **LONG_PRESS** - 长按

---

## 📡 通信协议

### ESP32 ← → STM32 消息格式

#### 1️⃣ STM32 → ESP32 (发送场景请求)

**格式**: `SCENE:<scene_key>[|IDX:<option_index>]\n`

**示例**:
```
SCENE:shopping\n              // 进入购物场景
SCENE:cafe|IDX:1\n           // 咖啡馆场景，选择第2个选项
```

**场景关键字**:
- `shopping` - 逛街约会
- `gaming` - 一起开黑
- `cafe` - 咖啡馆
- `library` - 图书馆
- `night` - 晚安夜聊
- `encourage` - 鼓励模式
- `walk` - 散步吹风
- `dinner` - 一起吃饭
- `movie` - 电影时间
- `music` - 音乐分享
- `exam` - 考前陪伴
- `rest` - 休息陪伴

#### 2️⃣ ESP32 → STM32 (发送AI回复)

**格式**: `TEXT=<reply_hex>|OPT1=<opt1_hex>|OPT2=<opt2_hex>|OPT3=<opt3_hex>|AVATAR=<avatar_type>\n`

**示例**:
```
TEXT=C4E3CFCBCBB5E3D2BB...|OPT1=CFCBC8B1|OPT2=...|OPT3=...|AVATAR=happy\n
```

**字段说明**:
- `TEXT`: AI回复文本 (GBK编码的16进制)
- `OPT1-3`: 三个选项 (GBK编码的16进制)
- `AVATAR`: 头像状态 (happy, shy, gentle, thinking, tired, curious)

**解析代码** (`μVision_AI_character/User/main.c`):
```c
Chat_ParseReply(char *buf)  // 解析上述格式的消息
```

---

## 📁 项目目录结构

```
Embodied-AI-Virtual-Character-on-Embedded-System/
│
├── 📁 esp32_bridge/
│   └── STM32_AI_Bridge/
│       └── STM32_AI_Bridge.ino          # ESP32网络网关代码
│
├── 📁 μVision_AI_character/
│   ├── User/
│   │   └── main.c                       # STM32主程序 (UI + 通信)
│   ├── Libraries/
│   │   ├── CMSIS/                       # STM32 HAL库
│   │   └── FWlib/                       # STM32 固件库
│   └── Doc/
│
├── 📁 facial/
│   ├── ai_avatars.c/h                   # 头像图像资源
│   └── avatar_*.png                     # 头像素材 (160x120)
│
├── 📁 gesture/
│   ├── gesture.c/h                      # 手势识别引擎
│
├── 📁 key_input/
│   ├── key_input.c/h                    # 按键处理
│
|—— 📁 audio/
│   ├── ...                           # 静态挂载文夹                    
├── 📁 xpt2046_ai_ready/
│   ├── bsp_xpt2046_lcd.c/h              # 触摸屏驱动
│   └── avatar_contact_sheet_160x120.png # 头像合并表
│
├── ESP32_Hello/
│   └── ESP32_Hello.ino                  # ESP32简单测试代码
│
├── main.py                              # Python后端 (可选)
|—— text_to_speech.py                    # 语音模块
└── README.md                            # 项目说明
```

---

## 🎯 关键功能模块

### 1. UI引擎 (main.c)

**页面状态**:
```c
typedef enum {
    PAGE_COVER,          // 封面页
    PAGE_SCENE_SELECT,   // 场景选择
    PAGE_CHAT            // 聊天交互
} AppPage;
```

**显示区域**:
- 头像区: (32, 24) - 176×132像素
- 对话框: (6, 162) - 228×80像素
- 选项区: (6, 248) - 228×69像素 (3个选项)
- 底部提示: (0, 298)

### 2. 情绪引擎

**支持的情绪状态**:
- `AVATAR_HAPPY` - 开心
- `AVATAR_SHY` - 害羞
- `AVATAR_GENTLE` - 温柔
- `AVATAR_THINKING` - 思考
- `AVATAR_TIRED` - 疲倦
- `AVATAR_CURIOUS` - 好奇

### 3. 手势识别

```c
typedef enum {
    GESTURE_NONE,
    GESTURE_TAP,          // 点击
    GESTURE_SWIPE_UP,     // 向上
    GESTURE_SWIPE_DOWN,   // 向下
    GESTURE_SWIPE_LEFT,   // 向左
    GESTURE_SWIPE_RIGHT,  // 向右
    GESTURE_LONG_PRESS    // 长按
} GestureType;
```

### 4. 串口通信

**收发函数** (main.c):
- `USART1_SendString()` - 发送字符串到ESP32
- `USART1_ReadLine()` - 从ESP32读一行数据 (带超时)
- `USART1_ClearRxBuffer()` - 清空接收缓冲区

---

## 🔄 通信流程示例

### 流程1: 用户选择场景

```
用户触摸"购物"场景
    ↓
[STM32] 发送: SCENE:shopping\n
    ↓
[ESP32] 接收 → 连接网络 → 调用云端API
    ↓
云端LLM 返回: AI回复 + 3个选项 + 头像类型
    ↓
[ESP32] 格式化为串口消息发回
    ↓
[STM32] 接收: TEXT=....|OPT1=....|OPT2=....|OPT3=....|AVATAR=happy\n
    ↓
解析GBK数据 → 显示头像 + 对话 + 选项
```

### 流程2: 用户选择回复选项

```
用户选择选项2 (K1翻页, K2确认)
    ↓
[STM32] 获取选项文本 (已缓存)
    ↓
[STM32] 发送: SCENE:shopping|IDX:1\n (IDX从0开始)
    ↓
[ESP32] 接收 → 更新历史记录 → 调用云端API (包含对话历史)
    ↓
重复流程1的后续步骤...
```

---

## 🖥️ Arduino IDE 环境配置与调试 (ESP32)

### 1. 安装 Arduino IDE

**方案 A: 在线安装 (推荐)**
1. 访问 [Arduino官网](https://www.arduino.cc/en/software)
2. 下载 **Arduino IDE 2.0+** 版本 (Windows/Mac/Linux)
3. 安装完成后启动 Arduino IDE

**方案 B: 便携版本**
- 直接下载便携版本，解压后运行 `arduino.exe` (无需管理员权限)

---

### 2. 添加 ESP32 开发板支持

**步骤 1: 打开开发板管理器**
- Arduino IDE → 菜单 `Tools` → `Board` → `Boards Manager...`
- 或快捷键: Ctrl+Shift+B

**步骤 2: 搜索并安装 ESP32 支持**

在 Boards Manager 搜索框中输入：
```
esp32
```

选择 `esp32` (由 Espressif Systems 官方维护)，点击 **Install** 最新版本

**推荐版本**: v2.0.0 或更新

**等待时间**: 首次安装需 2-5 分钟 (含下载编译工具链)

---

### 3. 安装必要的库

**步骤 1: 打开库管理器**
- Arduino IDE → 菜单 `Tools` → `Manage Libraries...`
- 或快捷键: Ctrl+Shift+I

**步骤 2: 安装所需库**

| 库名 | 用途 | 注意事项 |
|------|------|----------|
| ArduinoJson | JSON 解析 | `ArduinoJson` |
| WiFi | WiFi 连接 | 内置库 (无需安装) |
| HTTPClient | HTTP 请求 | 内置库 (无需安装) |
| ESP32-audioI2S-master | 扬声 | 3.0.13及以下版本 |


---

### 4. 配置开发板参数

**步骤 1: 选择开发板**

- Arduino IDE → 菜单 `Tools` → `Board` → 搜索框输入 `ESP32`
- 选择 **`ESP32 Dev Module`** (最通用, 兼容 ESP32-WRQOM-32E)

**步骤 2: 配置开发板选项**

菜单 `Tools` → 设置以下参数:

| 参数 | 设置值 | 说明 |
|------|--------|------|
| **Board** | ESP32 Dev Module | 开发板型号 |
| **Upload Speed** | 921600 | 烧录速度 (快) |
| **CPU Frequency** | 240 MHz | 主频 |
| **Flash Frequency** | 80 MHz | Flash频率 |
| **Flash Mode** | QIO | Flash模式 |
| **Flash Size** | 4MB | Flash大小 |
| **Partition Scheme** | HUGE APP | 分区方案 |
| **Core Debug Level** | Info | 调试级别 |
| **Port** | COM3 (或自动识别) | USB串口 |

**配置图示**:
```
Tools
├── Board → ESP32 Dev Module
├── Upload Speed → 921600
├── CPU Frequency → 240 MHz
├── Flash Frequency → 80 MHz
├── Flash Mode → QIO
├── Flash Size → 4MB
├── Partition Scheme → HUGE APP
├── Core Debug Level → Info
└── Port → COM3
```

---

### 5. 连接 ESP32 到 PC及MAX 98357A

**硬件连接**:

1. 用 **Micro-USB 数据线** 连接 ESP32 到 PC
2. LED 指示灯应该亮起 (红色/绿色)
3. 打开设备管理器检查串口是否识别
4. LRC-->25 BCLK-->26 DIN-->22 GND-->GND VIN-->3V3


**Windows 驱动**:
- 大多数 ESP32-WRQOM-32E 开发板集成 CH340 USB芯片
- 自动安装驱动，若未安装可从 [CH340官网](http://www.wch.cn/downloads/CH341SER_ZIP.html) 下载

**验证串口**:
- Windows: 设备管理器 → 端口 (COM和LPT) → 查看 `COM3` 或 `USB Serial Device`
- Mac: 终端运行 `ls /dev/tty.usbserial*`
- Linux: 终端运行 `ls /dev/ttyUSB*`

---

### 6. 打开项目代码

**步骤 1: 打开 Arduino IDE**

**步骤 2: 打开 ESP32 项目**
```
File → Open
导航到: esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino
```

或直接双击该文件打开

---

### 7. 编译和烧录

**步骤 1: 编译代码**

点击工具栏 **✓ 按钮** (Verify/Compile)

```
预期输出:
Compiling sketch...
Archiving built core (caching)...
Sketching built core...
Sketch uses 234567 bytes...
```

**步骤 2: 上传到 ESP32**

点击工具栏 **→ 按钮** (Upload)

```
预期输出:
Connecting........
Uploading...
A lot of dots (.........)
[==========] 100%
esptool.py v4.x done

Leaving...
Hard resetting via RTS pin...
```

**首次上传耗时**: 20-30 秒

**若连接失败**:
- 检查 USB 线是否完好
- 尝试按 ESP32 上的 **BOOT** 按钮进入下载模式
- 更新驱动或更换 USB 端口

---

### 8. 串口监视器 - 实时调试

**打开串口监视器**:

菜单 `Tools` → `Serial Monitor`

或快捷键: **Ctrl+Shift+M**

**配置参数**:

右下角设置:
- **波特率**: 115200
- **行结束符**: `Both NL & CR`

**预期输出** (ESP32启动):
```
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://10.129.215.6:8000/scene_story_serial
[bridge] connecting wifi...
```

**监视器技巧**:
- 勾选 **Autoscroll** - 自动滚动到最新消息
- 勾选 **Show timestamp** - 显示消息时间戳
- 输入框可发送AT命令或调试消息
- `Ctrl+L` 清空输出

---

### 9. 实时调试技巧

**运行时监视**:

使用 `Serial Monitor` 实时观看输出，快速定位问题:

```
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://10.129.215.6:8000/scene_story_serial
[bridge] connecting wifi...        ← WiFi连接中
........
[bridge] wifi ok, ip=192.168.1.100 ← WiFi连接成功
[RECV] SCENE:shopping              ← 接收STM32命令
[bridge] POST http://...           ← 调用API
[bridge] raw response={"reply":... ← API响应
[bridge] send serial=<gbk packet>  ← 发送回复
```

---

### 10. 常见问题排查

| 问题 | 症状 | 解决方案 |
|------|------|--------|
| **无法检测到开发板** | Port 下拉菜单为空 | 1. 检查USB线; 2. 重装CH340驱动; 3. 尝试USB HUB |
| **上传失败** | 出现 `timeout` 错误 | 按BOOT按钮, 或选择更低波特率 (115200) |
| **编译错误** | `undefined reference` | 检查库是否安装完整, 重启IDE |
| **乱码输出** | 串口监视器显示乱码 | 检查波特率设置是否为 115200 |
| **WiFi 无法连接** | 卡在 `connecting wifi...` | 检查SSID/密码, WiFi是否在2.4GHz |
| **API 无响应** | 无 HTTP 输出 | 检查网络连接, API地址是否正确 |
| **STM32 无回复** | 无 `[RECV]` 输出 | 检查串口接线, 波特率 |

---

### 11. 性能监视

**查看 ESP32 资源使用情况**:

在 `setup()` 中添加:

```cpp
Serial.print("Flash Size: ");
Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
Serial.println(" MB");

Serial.print("PSRAM Size: ");
Serial.print(ESP.getPsramSize() / 1024 / 1024);
Serial.println(" MB");
```

在 `loop()` 中定期输出:

```cpp
static unsigned long lastCheck = 0;
if (millis() - lastCheck > 10000) {  // 每10秒输出一次
  lastCheck = millis();
  Serial.print("[MONITOR] Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" bytes, WiFi RSSI: ");
  Serial.println(WiFi.RSSI());  // 信号强度
}
```

---

## ⚙️ WiFi 配置 (ESP32)

**配置位置**: `esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino`

```cpp
static const char *WIFI_SSID = "Redmi K70 Ultra";       // WiFi SSID
static const char *WIFI_PASS = "wkm4m5gpb6zgsmz";       // WiFi密码
static const char *API_BASE_URL = "http://10.129.215.6:8000";  // 后端地址
static const char *API_PATH = "/scene_story_serial";    // API路径
```

---

## 🐛 调试信息

### ESP32 Debug输出 (Serial @ 115200)
```
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://10.129.215.6:8000/scene_story_serial
[bridge] connecting wifi...
[bridge] wifi ok, ip=192.168.1.100
[bridge] recv frame=SCENE:shopping
[bridge] POST http://...
[bridge] send serial=<gbk packet>
```

---

