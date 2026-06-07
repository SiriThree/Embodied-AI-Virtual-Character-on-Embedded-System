# STM32 嵌入式 AI 虚拟角色系统

这是一个基于 `STM32F103VET6` 野火开发板的嵌入式 AI 虚拟角色项目。  
项目目标是在小尺寸触摸屏上实现一个具有“角色感、互动感、陪伴感”的 AI 伙伴系统。

当前仓库包含三部分：

- `STM32` 端：封面页、场景选择页、对话页、按键/触摸交互、头像与文本显示
- `ESP32` 端：负责串口桥接，把 STM32 请求转发给后端；连接扬声器播放交互语音
- `FastAPI + LLM + TTS` 后端：负责动态生成 AI 回复、用户选项和情绪状态、ai语音mp3回复

当前版本已经整理成可直接在 `Keil μVision` 中编译的模块化工程。

---

## 一、项目目录结构

```text
STM32_AI_Character/
├─ μVision_AI_character/                  # STM32 固件工程
│  ├─ Project/RVMDK（uv5）/BH-F103.uvprojx
│  └─ User/
│     ├─ main.c                           # 最小入口，仅保留启动与初始化
│     ├─ ai_app.c / ai_app.h              # 页面状态机、输入分发、待机逻辑
│     ├─ ai_ui.c / ai_ui.h                # 三个页面的 UI 绘制
│     ├─ ai_chat.c / ai_chat.h            # 串口协议、回包解析、mock 逻辑
│     ├─ ai_app_data.c / ai_app_data.h    # 场景数据、文案、布局常量
│     └─ ai_app_utils.c / ai_app_utils.h  # 文本裁切、换行、串口辅助函数
├─ esp32_bridge/
│  └─ STM32_AI_Bridge/
│     └─ STM32_AI_Bridge.ino              # ESP32 串口桥接程序
├─ docs/
│  └─ v3_modules_requirements.md          # 后续模块需求文档
├─ main.py                                # FastAPI 后端入口
├─ ai_partner_memory.db                   # 本地记忆数据库
|─ text_to_speech.py                      # Text to Speech
|-audio/                                  # 挂载mp3静态文件
└─ README.md
```

---

## 二、STM32 工程编译说明

### 1. 开发环境

- `Keil MDK-ARM / μVision5`
- `ARM Compiler 5`

当前验证通过的编译器版本：

- `V5.06 update 6 (build 750)`

### 2. 工程位置

请在 `μVision` 中打开：

```text
μVision_AI_character/Project/RVMDK（uv5）/BH-F103.uvprojx
```

工程关键信息：

- Target：`LDC`
- Device：`STM32F103VE`
- Output：`Template`

### 3. 编译步骤

1. 打开 `BH-F103.uvprojx`
2. 确认当前 Target 为 `LDC`
3. 点击 `Rebuild`

当前工程文件已经包含拆分后的新模块，不需要手动再往工程里加文件。

---

## 三、STM32 端当前功能

### 1. 页面流程

系统当前流程为：

1. 封面页
2. 场景/话题选择页
3. 多轮对话页

### 2. 交互方式

- `K1 短按`：切换场景或切换选项
- `K1 长按`：反向切换
- `K2 短按`：确认当前选择
- `K2 长按`：返回上一页
- 触摸点击：选择场景或选择选项
- 触摸滑动：场景翻动或页面切换

### 3. 当前内置场景

当前已内置多个场景，例如：

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
- 考试前夕
- 休息陪伴

### 4. 本地兜底逻辑

如果 STM32 没有接上 ESP32，或者后端没有连通，系统仍然可以使用本地 mock 回复进行演示，不会卡死。

---

## 四、STM32 模块职责说明

### `main.c`

只保留两类职责：

- `main()`
- 硬件初始化入口

### `ai_app.c`

负责：

- 页面状态机
- 按键与触摸输入分发
- 场景切换
- 对话选项切换
- 待机反馈逻辑

### `ai_ui.c`

负责：

- 封面页绘制
- 场景页绘制
- 对话页绘制
- 头像缩放显示
- 对话文本与选项显示

### `ai_chat.c`

负责：

- 进入场景后的首轮请求
- 选项发送
- 串口回包解析
- 本地 mock 回复
- avatar 状态解析

### `ai_app_data.c`

负责：

- 场景表
- UI 中文文案
- 待机提示文案
- 布局与颜色常量

### `ai_app_utils.c`

负责：

- 文本按像素宽度裁切
- 自动换行
- 省略号处理
- 字符串拼接
- 串口收发辅助

---

## 五、ESP32 串口桥接说明

桥接文件：

```text
esp32_bridge/STM32_AI_Bridge/STM32_AI_Bridge.ino
```

### 1. 作用

ESP32 负责接收 STM32 发来的串口请求，访问 FastAPI 后端，再把结果通过串口发回 STM32。

### 2. 串口协议

STM32 -> ESP32：

```text
SCENE:shopping
SCENE:shopping|IDX:0
SCENE:gaming|IDX:2
```

ESP32 -> STM32：

```text
TEXT=...|OPT1=...|OPT2=...|OPT3=...|AVATAR=happy
```

### 3. 烧录前需要确认的配置

请检查 `.ino` 中以下内容：

- `WIFI_SSID`
- `WIFI_PASS`
- `API_BASE_URL`
- `API_PATH`

### 4. 推荐接线

当前 STM32 工程使用 `USART2`：

- `STM32 PA2 = TX`
- `STM32 PA3 = RX`

ESP32 桥接默认使用：

- `GPIO16 = RX`
- `GPIO17 = TX`

连接方式：

- `ESP32 GPIO17 (TX) -> STM32 PA3 (RX)`
- `ESP32 GPIO16 (RX) -> STM32 PA2 (TX)`
- `GND -> GND`



### 5. 串口监视器

波特率：

```text
115200
```

正常启动后应看到类似日志：

```text
[bridge] boot
[bridge] uart2 baud=115200
[bridge] api=http://你的电脑IP:8000/scene_story_serial
[bridge] connecting wifi...
[bridge] wifi ok, ip=...
```

### 6. 语音模块

语音模块接线：
- 'LRC-->25' 
- 'BCLK-->26'
- 'DIN-->22'
- 'GND-->GND'
- 'VIN-->3V3'

注意事项：
- 安装ESP32-audioI2S-master(3.0.13版本及以前)
- tool选项卡partition scheme选择"HUGE APP"以获得充足内存
---

## 六、FastAPI 后端说明

后端入口文件：

```text
main.py
```

### 1. Python 环境

推荐：

- `Python 3.10+`

### 2. 安装依赖

仓库现在已经补充了 `requirements.txt`，可直接执行：

```bash
pip install -r requirements.txt
```

### 3. 环境变量

先复制：

```text
.env.example -> .env
```

然后在 `.env` 中填写：

```text
DEEPSEEK_API_KEY=你的密钥
```

### 4. 启动方式

在仓库根目录运行：

```bashuvicorn main:app --host 0.0.0.0 --port 8000

```

启动后可访问：

```text
http://127.0.0.1:8000/docs
```

### 5. 当前主要接口

- `POST /scene_serial`
- `POST /scene_story_serial`
- `POST /chat`

实际联调推荐使用：

- `POST /scene_story_serial`

---

## 七、最短上手流程

### 方案 A：只验证 STM32 UI

1. 用 `μVision` 打开工程
2. 编译并烧录 STM32
3. 检查封面页、场景页、对话页是否正常显示

### 方案 B：STM32 + ESP32

1. 编译并烧录 STM32
2. 配置并烧录 ESP32 `.ino`
3. 接好串口线
4. 观察 ESP32 串口监视器日志

### 方案 C：完整联调

1. 启动 FastAPI
2. 烧录 ESP32
3. 烧录 STM32
4. 进入任意场景
5. 检查动态回复、动态选项和头像状态是否正常

---

## 八、常见问题

### 1. μVision 能编译，但烧录后没反应

优先检查：

- `ST-Link` 连接
- 板子供电
- `Connect under Reset`
- 下载算法是否正确

### 2. ESP32 串口监视器出现 `http failed, status=-1`

通常是后端地址或局域网问题，请检查：

- `API_BASE_URL`
- 电脑当前局域网 IP
- `uvicorn` 是否使用 `0.0.0.0`
- Windows 防火墙是否放行 `8000`

### 3. STM32 屏幕内容与 ESP32 日志不一致

优先检查：

- TX/RX 是否交叉连接
- 是否使用当前仓库里的 `USART2` 版本固件
- STM32 是否真的烧录到最新固件

### 4. 中文出现乱码

当前 STM32 侧依赖 `GBK` 路径显示中文。若出现乱码，请检查：

- STM32 是否烧录了最新固件
- ESP32 bridge 是否与当前仓库版本一致
- 串口回包是否仍符合当前协议

---

## 九、当前项目状态

当前仓库更偏向“可演示原型”，而不是最终产品封装版。

已经具备：

- STM32 模块化 UI 与交互逻辑
- ESP32 串口桥接
- FastAPI + LLM 动态对话
- 场景化多轮选项交互
- ESP32 GET云端mp3文件 + MAX98357A播放

后续可继续扩展：

- 好感度与情绪记忆模块
- 视觉感知模块
- 剧情分支模块

---

## 十、建议提交前自检

如果你准备把仓库交给其他人使用，建议先确认：

1. `BH-F103.uvprojx` 能在本机 `μVision` 正常打开
2. `LDC` Target 能直接编译通过
3. `.env.example` 内容完整
4. `STM32_AI_Bridge.ino` 中的 Wi-Fi 和 IP 配置清晰
5. README 中的路径与仓库结构一致

