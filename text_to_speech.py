import os
import asyncio
from pathlib import Path
import edge_tts

AUDIO_DIR = "./audio"
VOICE = "zh-CN-XiaoxiaoNeural"
EMOTION_VOICE_MAP = {
    "happy": {"voice": VOICE, "rate": "+20%", "pitch": "+5Hz"},
    "shy": {"voice": VOICE, "rate": "-10%", "pitch": "+2Hz"},
    "gentle": {"voice": VOICE, "rate": "-5%", "pitch": "+0Hz"},
    "thinking": {"voice": VOICE, "rate": "-15%", "pitch": "-2Hz"},
    "curious": {"voice": VOICE, "rate": "+10%", "pitch": "+3Hz"}
}

def ensure_audio_dir():
    Path(AUDIO_DIR).mkdir(parents=True, exist_ok=True)

def get_next_audio_index() -> int:
    ensure_audio_dir()
    files = [f for f in os.listdir(AUDIO_DIR) if f.endswith('.mp3')]

    indices = []
    for f in files:
        try:
            idx = int(f.replace('.mp3', ''))
            indices.append(idx)
        except ValueError:
            pass

    if not indices:
        return 0

    max_idx = max(indices)

    if max_idx >= 9:
        for f in files:
            try:
                os.remove(os.path.join(AUDIO_DIR, f))
            except:
                pass
        return 0

    return max_idx + 1

def get_audio_filename(index: int) -> str:
    return f"{index:02d}.mp3"

async def text_to_speech(text: str, emotion: str = "gentle") -> str:
    ensure_audio_dir()

    voice_config = EMOTION_VOICE_MAP.get(emotion, EMOTION_VOICE_MAP["gentle"])
    voice = voice_config.get("voice")
    rate = voice_config.get("rate", "+0%")
    pitch = voice_config.get("pitch", "+0Hz")

    index = get_next_audio_index()
    audio_file = get_audio_filename(index)
    audio_path = os.path.join(AUDIO_DIR, audio_file)

    try:
        communicate = edge_tts.Communicate(
            text=text,
            voice=voice,
            rate=rate,
            pitch=pitch
        )
        await communicate.save(audio_path)
        return audio_path, audio_file
    except Exception as e:
        print(f"TTS Error: {e}")
        return None, None

def get_audio_url(audio_file: str, base_url: str = "http://192.168.24.231:8000") -> str:
    if not audio_file:
        return ""
    return f"{base_url}/audio/{audio_file}"




# def sync_text_to_speech(text: str, emotion: str = "gentle") -> tuple:
#     loop = asyncio.new_event_loop()
#     asyncio.set_event_loop(loop)
#     try:
#         result = loop.run_until_complete(text_to_speech(text, emotion))
#         return result
#     finally:
#         loop.close()
