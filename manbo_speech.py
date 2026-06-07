import os
import requests
from pathlib import Path
from time import time
import urllib3
from dotenv import load_dotenv

# 配置信息
load_dotenv()
AUDIO_DIR = "./audio"
FISH_AUDIO_API_KEY = os.getenv("FISH_AUDIO_API_KEY") 
FISH_AUDIO_URL = "https://api.fish.audio/v1/tts"

# 情感与模型 ID 的映射表
EMOTION_MODEL_MAP = {
    "shy": "3d5cffb9b214466793af82e5e5670622",
    "gentle": "253fc27b8b104512bc136357076aadef",
    "curious": "166fe81ca9fd461f92d484644e0b1caa",
    "thinking": "d7a09707236343e2b91754bd1c7c38cf",
    "happy": "0f08cacd3e354471a4b94dd00b4cc4a3"
}

def ensure_audio_dir():
    """确保音频目录存在"""
    Path(AUDIO_DIR).mkdir(parents=True, exist_ok=True)

def get_next_audio_index() -> int:
    """获取下一个音频索引，并执行 09 清空规则"""
    ensure_audio_dir()
    files = [f for f in os.listdir(AUDIO_DIR) if f.endswith('.mp3')]
    
    indices = []
    for f in files:
        try:
            # 提取文件名数字部分
            idx = int(f.replace('.mp3', ''))
            indices.append(idx)
        except ValueError:
            pass

    if not indices:
        return 0

    max_idx = max(indices)

    # 如果索引达到或超过 9，清空文件夹并重置为 0
    if max_idx >= 9:
        for f in files:
            try:
                os.remove(os.path.join(AUDIO_DIR, f))
            except Exception as e:
                print(f"删除文件失败: {e}")
        return 0

    return max_idx + 1

def get_audio_filename(index: int) -> str:
    return f"{index:02d}.mp3"

# 禁用安全请求警告
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# 创建全局 Session 对象，可以复用 TCP 连接，提高稳定性
session = requests.Session()

async def text_to_speech(text: str, emotion: str = "gentle") -> tuple:
    ensure_audio_dir()
    ref_id = EMOTION_MODEL_MAP.get(emotion, EMOTION_MODEL_MAP["gentle"])
    index = get_next_audio_index()
    audio_file = get_audio_filename(index)
    audio_path = os.path.join(AUDIO_DIR, audio_file)

    headers = {
        "Authorization": f"Bearer {FISH_AUDIO_API_KEY}",
        "Content-Type": "application/json"
    }
    payload = {
        "text": text,
        "reference_id": ref_id,
        "format": "mp3"
    }

    max_retries = 3  # 最大重试次数
    for attempt in range(max_retries):
        try:
            # 使用 session 发送请求，verify=False 解决证书问题
            response = session.post(
                FISH_AUDIO_URL, 
                json=payload, 
                headers=headers, 
                timeout=20, 
                verify=False
            )
            
            if response.status_code == 200:
                with open(audio_path, "wb") as f:
                    f.write(response.content)
                return audio_path, audio_file
            else:
                print(f"API 报错 (尝试 {attempt+1}/{max_retries}): {response.status_code}")
        
        except (requests.exceptions.SSLError, requests.exceptions.ConnectionError) as e:
            print(f"网络抖动 (尝试 {attempt+1}/{max_retries}): {e}")
            if attempt < max_retries - 1:
                time.sleep(1)  # 等待1秒后重试
            continue
        except Exception as e:
            print(f"未知错误: {e}")
            break

    return None, None

def get_audio_url(audio_file: str, base_url: str = "http://192.168.24.231:8000") -> str:
    """生成音频的访问 URL"""
    if not audio_file:
        return ""
    return f"{base_url}/audio/{audio_file}"