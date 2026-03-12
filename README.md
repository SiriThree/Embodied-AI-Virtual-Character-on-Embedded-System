# Embodied AI Virtual Pet

An embedded AI virtual character system running on STM32 with a touchscreen interface.  
The project integrates emotion dynamics, gesture interaction, real-time animation, and cloud-based large language models to create a lifelike AI companion.

## ✨ Features

- **Embodied AI Character**
  - Animated virtual character with dynamic behaviors
  - Emotion-driven interaction and responses

- **Emotion Modeling System**
  - Multi-dimensional emotional state
  - Emotions evolve over time and interactions
  - Emotion affects behavior and animation

- **Gesture Interaction (TinyML)**
  - Touch trajectory recognition
  - Supports gesture-based interaction
  - Lightweight model running on embedded hardware

- **AI Conversation**
  - Cloud-based LLM interaction
  - Context-aware dialogue
  - Personality-driven responses

- **Real-time Animation Engine**
  - Sprite-based character animation
  - Emotion-based animation switching

- **Touchscreen Interface**
  - Interactive GUI built for embedded devices
  - Real-time feedback and visual response

---

## 🧠 System Architecture
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

## 🖥 Hardware

- STM32F103VET6
- Touchscreen LCD display
- Serial communication module (UART / WiFi)

---

## 🧩 Software Stack

Embedded Side:

- C / C++
- LVGL GUI Library
- TinyML inference

Edge AI Service:

- Python
- LLM API
- Emotion & behavior controller

---

## 🎮 Interaction Examples

User touches the character  
→ character becomes happy  

User draws a gesture  
→ gesture recognition triggers animation  

User leaves the device idle  
→ AI character becomes bored and initiates conversation  

---

## 📂 Project Structure
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

## 🚀 Getting Started

### Firmware

1. Open project in STM32CubeIDE
2. Compile firmware
3. Flash to STM32 board

### AI Service
cd edge_ai
pip install -r requirements.txt
python server.py


---

## 📊 Future Improvements

- Voice interaction
- On-device TinyML emotion recognition
- Reinforcement learning for behavior evolution
- Multi-character interaction

---
