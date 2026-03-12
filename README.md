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
- 串行通信模块（UART / WiFi）

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

---

## 📂 项目结构
project/

├── firmware/

│ ├── drivers/

│ ├── ui/

│ ├── animation/

│ └── ai/

│

├── edge_ai/

│ ├── agent/

│ ├── memory/

│ └── dialogue/

│

├── docs/

│ ├── architecture.md

│ └── protocol.md

│

└── README.md


---

## 🚀 快速开始

### 固件部署

1. 在 STM32CubeIDE 中打开项目
2. 编译固件代码
3. 将固件烧录至 STM32 开发板

### AI Service
cd edge_ai

pip install -r requirements.txt

python server.py


---

## 📊 未来优化方向

- 语音交互功能
- 设备端 TinyML 情绪识别
- 基于强化学习的行为演变机制
- 多角色交互能力

---
