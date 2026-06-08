import os
import json
import sqlite3
import re
from datetime import datetime
from typing import List, Dict, Optional, Tuple

from dotenv import load_dotenv
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from openai import OpenAI
#from text_to_speech import text_to_speech, get_audio_url
from manbo_speech import text_to_speech,get_audio_url
# 
# =========================
# 基础配置
# =========================

load_dotenv()

DEEPSEEK_API_KEY = os.getenv("DEEPSEEK_API_KEY")
DATABASE_PATH = "ai_partner_memory.db"

if not DEEPSEEK_API_KEY:
    raise RuntimeError("请先在 .env 文件中配置 DEEPSEEK_API_KEY")

client = OpenAI(
    api_key=DEEPSEEK_API_KEY,
    base_url="https://api.deepseek.com"
)

app = FastAPI()

# 挂载audio静态文件夹
app.mount("/audio", StaticFiles(directory="audio"), name="audio")



# =========================
# 场景分支互动接口
# =========================

class SceneRequest(BaseModel):
    scene_name: str
    scene_description: str
    ai_intro: str
    user_id: str = "default_user"
    history: List[str] = []

class SceneResponse(BaseModel):
    reply: str
    reply_gbk_hex: str
    user_options: List[str]
    options_gbk_hex: List[str]
    avatars: List[str]
    audio_url: str = ""
    serial_line: str = ""

def encode_gbk_hex(text: str) -> str:
    return text.encode("gbk", errors="ignore").hex()


VALID_AVATARS = {"happy", "shy", "gentle", "thinking", "curious"}


def normalize_text(text: str, fallback: str, max_len: int) -> str:
    cleaned = (text or "").replace("\r", " ").replace("\n", " ").replace("|", " ")
    cleaned = re.sub(r"\s+", " ", cleaned).strip()
    if not cleaned:
        cleaned = fallback
    return cleaned[:max_len]


def normalize_avatar(token: str) -> str:
    token = (token or "").strip().lower()
    if token in VALID_AVATARS:
        return token
    return "gentle"


def normalize_options(options: List[str], scene_name: str) -> List[str]:
    fallback_options = [
        f"先陪我逛逛{scene_name}"[:14],
        "曼波曼波一下",
        "换个甜甜话题",
    ]
    normalized = []
    for idx in range(3):
        raw = options[idx] if idx < len(options) else fallback_options[idx]
        normalized.append(normalize_text(raw, fallback_options[idx], 18))
    return normalized


def build_serial_line(reply: str, options: List[str], avatar: str, audio_url: str = "") -> str:
    line = (
        f"TEXT={reply}|"
        f"OPT1={options[0]}|"
        f"OPT2={options[1]}|"
        f"OPT3={options[2]}|"
        f"AVATAR={avatar}"
    )
    if audio_url:
        line += f"|AUDIO={audio_url}"
    return line


def extract_json_object(content: str) -> Dict:
    content = content.strip()
    match = re.search(r"\{.*\}", content, flags=re.S)
    if match:
        content = match.group(0)
    return json.loads(content)


def normalize_story_options(options: List[str], scene_name: str) -> List[str]:
    fallback_options = [
        f"先陪我逛逛{scene_name}"[:14],
        "来点甜甜互动",
        "把剧情推推看",
    ]
    normalized = []
    for idx in range(3):
        raw = options[idx] if idx < len(options) else fallback_options[idx]
        normalized.append(normalize_text(raw, fallback_options[idx], 18))
    return normalized


def build_story_prompt(req: "SceneRequest", history_text: str) -> str:
    return f"""
你现在扮演“哈基米曼波”，一位运行在嵌入式恋爱互动界面里的 AI 伙伴。
你要贴近全网最常见的二创印象：软萌、呆呆的、慢半拍、甜甜的、魔性轻哼唱、带一点无厘头抽象感。
你的目标不是普通聊天，而是把互动写成带有轻陪伴、轻喜感的有趣小分支。

当前场景：{req.scene_name}
场景描述：{req.scene_description}
开场设定：{req.ai_intro}
最近对话：{history_text}

请严格遵守这些规则：
1. 所有输出必须是简体中文。
2. 只返回 JSON，不要解释，不要 markdown，不要代码块。
3. reply 是哈基米曼波此刻说的话：
   - 控制在 10 到 24 个汉字左右
   - 以短句为主，软糯、轻快、慢悠悠
   - 可以带一点“哈基米～”“曼波曼波～”这类轻口头禅
   - 可以轻轻发呆、卖萌、打趣，但不要成熟、说教、官方
   - 要承接最近对话，不能像重新开场
   - 整体要像一个呆萌、治愈、幽默中带点腹黑的网红小萌物
4. user_options 必须正好 3 条，且每条都明显不同：
   - 第 1 条：温柔回应 / 拉近关系
   - 第 2 条：调皮回应 / 呆萌打趣
   - 第 3 条：推进剧情 / 转入下一步互动
5. 每条 user_option 控制在 6 到 14 个汉字左右，要短、顺口、像小屏幕上的游戏选项。
6. 选项不能只是换个说法重复同一个意思，不能机械复读上一轮内容。
7. 整体氛围参考：可爱，呆呆的、萌萌的、傻傻的松弛感
8. avatar 只能是 happy、shy、gentle、thinking、curious 之一。

返回格式必须严格是：
{{"reply":"...", "user_options":["...", "...", "..."], "avatar":"happy"}}
"""

@app.post("/scene", response_model=SceneResponse)
async def generate_scene(req: SceneRequest):
    """
    根据场景信息动态生成哈基米曼波风格的 AI 回复、用户选项和表情状态
    """
    prompt = f"""
你是“哈基米曼波”，一个软萌、呆呆的、会轻轻哼唱的二创虚拟少女。
你要根据场景生成适合嵌入式小屏幕显示的互动文案，气质要甜、轻、治愈、慢悠悠。

场景名称：{req.scene_name}
场景描述：{req.scene_description}
AI 初始台词：{req.ai_intro}
历史对话：{req.history}

请根据场景生成：
1. 一句简短的 AI 回复（1句，8~18个汉字，口吻要像哈基米曼波）
2. 3 个合理的用户可能回复选项，每个 8~12 个汉字，符合该场景互动逻辑
3. 每条 AI 回复对应的表情状态（happy, shy, gentle, thinking, curious 等）

额外要求：
- 说话不要成熟，不要理性分析，不要像客服
- 可以轻轻带一点“哈基米～”“曼波曼波～”这类轻口头禅
- 整体像短视频和表情包里的呆萌治愈感

输出 JSON 格式：
{{"reply": "...", "user_options": ["...","...","..."], "avatars": ["...","...","..."]}}
"""

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[{"role": "user", "content": prompt}],
            temperature=0.7,
            max_tokens=200,
            stream=False
        )
        content = response.choices[0].message.content.strip()
        import json
        data = json.loads(content)

        reply = data.get("reply", "哈基米～南北绿豆~")
        user_options = data.get("user_options", ["贴贴一下","曼波摇摇","继续陪我"])
        avatars = data.get("avatars", ["happy","happy","happy"])

        # 生成语音，根据第一个avatar
        primary_avatar = avatars[0] if avatars else "gentle"
        audio_path, audio_file = await text_to_speech(reply, primary_avatar)
        audio_url = get_audio_url(audio_file) if audio_file else ""
        serial_line = build_serial_line(reply, user_options, primary_avatar, audio_url)

        return SceneResponse(
            reply=reply,
            reply_gbk_hex=encode_gbk_hex(reply),
            user_options=user_options,
            options_gbk_hex=[encode_gbk_hex(opt) for opt in user_options],
            avatars=avatars,
            audio_url=audio_url,
            serial_line=serial_line
        )

    except Exception as e:
        default_reply = "哈基米～南北绿豆~"
        default_options = ["贴贴一下","曼波摇摇","继续陪我"]
        default_avatars = ["happy","happy","happy"]

        # 生成语音
        audio_path, audio_file = await text_to_speech(default_reply, "happy")
        audio_url = get_audio_url(audio_file) if audio_file else ""
        serial_line = build_serial_line(default_reply, default_options, "happy", audio_url)

        return SceneResponse(
            reply=default_reply,
            reply_gbk_hex=encode_gbk_hex(default_reply),
            user_options=default_options,
            options_gbk_hex=[encode_gbk_hex(opt) for opt in default_options],
            avatars=default_avatars,
            audio_url=audio_url,
            serial_line=serial_line
        )


# =========================
# 数据模型
# =========================

class SceneSerialResponse(BaseModel):
    reply: str
    reply_gbk_hex: str
    user_options: List[str]
    options_gbk_hex: List[str]
    avatar: str
    serial_line: str
    audio_url: str = ""


@app.post("/scene_serial", response_model=SceneSerialResponse)
async def generate_scene_serial(req: SceneRequest):
    history_text = " / ".join(req.history[-6:]) if req.history else "(none)"
    prompt = f"""
You are an anime-style AI companion in a scene-based interactive UI.
Return strict JSON only, and all visible dialogue must be Simplified Chinese.

Scene name: {req.scene_name}
Scene description: {req.scene_description}
AI intro: {req.ai_intro}
Recent history: {history_text}

Rules:
- reply: one natural AI reply in Simplified Chinese, about 10-24 Chinese characters
- user_options: exactly 3 short candidate replies in Simplified Chinese
- keep the three options meaningfully different:
  1. gentle / caring
  2. playful / teasing
  3. push the interaction or topic forward
- each option should be about 6-16 Chinese characters
- avoid repeating the exact same wording from recent history
- avatar must be exactly one of: happy, shy, gentle, thinking, curious
- no markdown, no explanation, JSON only

Return this schema exactly:
{{"reply":"...", "user_options":["...", "...", "..."], "avatar":"happy"}}
"""

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[{"role": "user", "content": prompt}],
            temperature=0.9,
            max_tokens=220,
            stream=False
        )
        content = response.choices[0].message.content.strip()
        data = extract_json_object(content)

        reply = normalize_text(data.get("reply"), "哈基米～我们继续呀。", 32)
        user_options = normalize_options(data.get("user_options", []), req.scene_name)
        avatar = normalize_avatar(data.get("avatar"))
    except Exception:
        reply = normalize_text(
            f"曼波曼波，我们继续在{req.scene_name}里呀。",
            "哈基米～继续聊呀。",
            32,
        )
        user_options = normalize_options(
            [
                f"先陪我逛逛{req.scene_name}",
                "来点曼波互动呀",
                "把话题继续展开",
            ],
            req.scene_name,
        )
        avatar = "gentle"

    # 生成语音
    audio_path, audio_file = await text_to_speech(reply, avatar)
    audio_url = get_audio_url(audio_file) if audio_file else ""

    return SceneSerialResponse(
        reply=reply,
        reply_gbk_hex=encode_gbk_hex(reply),
        user_options=user_options,
        options_gbk_hex=[encode_gbk_hex(opt) for opt in user_options],
        avatar=avatar,
        serial_line=build_serial_line(reply, user_options, avatar, audio_url),
        audio_url=audio_url,
    )


@app.post("/scene_story_serial", response_model=SceneSerialResponse)
async def generate_scene_story_serial(req: SceneRequest):
    history_text = " / ".join(req.history[-8:]) if req.history else "(none)"
    prompt = build_story_prompt(req, history_text)

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[{"role": "user", "content": prompt}],
            temperature=1.0,
            max_tokens=260,
            stream=False,
        )
        content = response.choices[0].message.content.strip()
        data = extract_json_object(content)

        reply = normalize_text(data.get("reply"), "哈基米～我们继续呀。", 32)
        user_options = normalize_story_options(data.get("user_options", []), req.scene_name)
        avatar = normalize_avatar(data.get("avatar"))
    except Exception:
        reply = normalize_text(
            f"曼波曼波，我们继续在{req.scene_name}里呀。",
            "哈基米～继续聊呀。",
            32,
        )
        user_options = normalize_story_options(
            [
                f"先陪我逛逛{req.scene_name}",
                "来点甜甜互动呀",
                "把剧情往下推呀",
            ],
            req.scene_name,
        )
        avatar = "gentle"

    # 生成语音
    audio_path, audio_file = await text_to_speech(reply, avatar)
    audio_url = get_audio_url(audio_file) if audio_file else ""

    return SceneSerialResponse(
        reply=reply,
        reply_gbk_hex=encode_gbk_hex(reply),
        user_options=user_options,
        options_gbk_hex=[encode_gbk_hex(opt) for opt in user_options],
        avatar=avatar,
        serial_line=build_serial_line(reply, user_options, avatar, audio_url),
        audio_url=audio_url,
    )



class ChatRequest(BaseModel):
    message: str
    user_id: Optional[str] = "default_user"


class ChatResponse(BaseModel):
    reply: str
    reply_gbk_hex: str
    emotion: str
    mood: str
    animation: str
    audio_url: str = ""


# =========================
# 数据库初始化
# =========================

def get_conn():
    return sqlite3.connect(DATABASE_PATH)


def init_db():
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS chat_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id TEXT NOT NULL,
        role TEXT NOT NULL,
        content TEXT NOT NULL,
        created_at TEXT NOT NULL
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS long_term_memory (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id TEXT NOT NULL,
        memory_type TEXT NOT NULL,
        content TEXT NOT NULL,
        importance INTEGER DEFAULT 1,
        created_at TEXT NOT NULL,
        updated_at TEXT NOT NULL
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS user_profile (
        user_id TEXT PRIMARY KEY,
        nickname TEXT,
        current_emotion TEXT DEFAULT 'neutral',
        recent_state TEXT,
        preferences TEXT,
        updated_at TEXT NOT NULL
    )
    """)

    conn.commit()
    conn.close()


init_db()


# =========================
# 基础存储函数
# =========================

def save_message(user_id: str, role: str, content: str):
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        INSERT INTO chat_history (user_id, role, content, created_at)
        VALUES (?, ?, ?, ?)
        """,
        (user_id, role, content, datetime.now().isoformat())
    )

    conn.commit()
    conn.close()


def get_recent_history(user_id: str, limit: int = 12) -> List[Dict[str, str]]:
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT role, content
        FROM chat_history
        WHERE user_id = ?
        ORDER BY id DESC
        LIMIT ?
        """,
        (user_id, limit)
    )

    rows = cursor.fetchall()
    conn.close()

    rows.reverse()

    return [
        {
            "role": role,
            "content": content
        }
        for role, content in rows
    ]


def get_chat_count(user_id: str) -> int:
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT COUNT(*)
        FROM chat_history
        WHERE user_id = ?
        """,
        (user_id,)
    )

    count = cursor.fetchone()[0]
    conn.close()

    return count


# =========================
# 长期记忆函数
# =========================

def save_memory(user_id: str, memory_type: str, content: str, importance: int = 1):
    content = content.strip()

    if not content:
        return

    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT id
        FROM long_term_memory
        WHERE user_id = ? AND content = ?
        """,
        (user_id, content)
    )

    exists = cursor.fetchone()
    now = datetime.now().isoformat()

    if exists:
        cursor.execute(
            """
            UPDATE long_term_memory
            SET updated_at = ?, importance = MAX(importance, ?)
            WHERE id = ?
            """,
            (now, importance, exists[0])
        )
    else:
        cursor.execute(
            """
            INSERT INTO long_term_memory
            (user_id, memory_type, content, importance, created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (user_id, memory_type, content, importance, now, now)
        )

    conn.commit()
    conn.close()


def get_long_term_memories(user_id: str, limit: int = 30) -> List[str]:
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT memory_type, content
        FROM long_term_memory
        WHERE user_id = ?
        ORDER BY importance DESC, updated_at DESC
        LIMIT ?
        """,
        (user_id, limit)
    )

    rows = cursor.fetchall()
    conn.close()

    return [f"[{memory_type}] {content}" for memory_type, content in rows]


# =========================
# 用户画像 Profile
# =========================

def ensure_profile(user_id: str):
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT user_id
        FROM user_profile
        WHERE user_id = ?
        """,
        (user_id,)
    )

    exists = cursor.fetchone()

    if not exists:
        cursor.execute(
            """
            INSERT INTO user_profile
            (user_id, nickname, current_emotion, recent_state, preferences, updated_at)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (user_id, None, "neutral", "", "", datetime.now().isoformat())
        )

    conn.commit()
    conn.close()


def get_profile(user_id: str) -> Dict[str, str]:
    ensure_profile(user_id)

    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT nickname, current_emotion, recent_state, preferences, updated_at
        FROM user_profile
        WHERE user_id = ?
        """,
        (user_id,)
    )

    row = cursor.fetchone()
    conn.close()

    nickname, current_emotion, recent_state, preferences, updated_at = row

    return {
        "nickname": nickname or "",
        "current_emotion": current_emotion or "neutral",
        "recent_state": recent_state or "",
        "preferences": preferences or "",
        "updated_at": updated_at or ""
    }


def update_profile(
    user_id: str,
    nickname: Optional[str] = None,
    current_emotion: Optional[str] = None,
    recent_state: Optional[str] = None,
    preferences: Optional[str] = None
):
    ensure_profile(user_id)

    old = get_profile(user_id)

    new_nickname = nickname if nickname is not None else old["nickname"]
    new_emotion = current_emotion if current_emotion is not None else old["current_emotion"]
    new_state = recent_state if recent_state is not None else old["recent_state"]
    new_preferences = preferences if preferences is not None else old["preferences"]

    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        """
        UPDATE user_profile
        SET nickname = ?, current_emotion = ?, recent_state = ?, preferences = ?, updated_at = ?
        WHERE user_id = ?
        """,
        (
            new_nickname,
            new_emotion,
            new_state,
            new_preferences,
            datetime.now().isoformat(),
            user_id
        )
    )

    conn.commit()
    conn.close()


# =========================
# 简单规则：用户输入分析
# =========================

def detect_emotion(text: str) -> str:
    anxiety_words = ["焦虑", "慌", "紧张", "压力", "不安", "担心", "害怕"]
    tired_words = ["累", "困", "疲惫", "没力气", "没动力", "颓废", "不想动"]
    sad_words = ["难过", "伤心", "委屈", "失落", "孤独", "想哭", "崩溃"]
    angry_words = ["烦", "气", "生气", "火大", "无语", "讨厌"]
    happy_words = ["开心", "高兴", "爽", "舒服", "快乐", "好耶", "哈哈"]
    love_words = ["喜欢你", "想你", "陪我", "抱抱", "亲密"]

    if any(w in text for w in anxiety_words):
        return "anxious"
    if any(w in text for w in tired_words):
        return "tired"
    if any(w in text for w in sad_words):
        return "sad"
    if any(w in text for w in angry_words):
        return "irritated"
    if any(w in text for w in happy_words):
        return "happy"
    if any(w in text for w in love_words):
        return "attached"

    return "neutral"


def select_animation(emotion: str) -> Tuple[str, str]:
    mapping = {
        "anxious": ("gentle", "comfort"),
        "tired": ("soft", "soft_smile"),
        "sad": ("warm", "comfort"),
        "irritated": ("calm", "listen"),
        "happy": ("bright", "happy_smile"),
        "attached": ("sweet", "soft_smile"),
        "neutral": ("natural", "idle")
    }

    return mapping.get(emotion, ("natural", "idle"))


def extract_rule_based_memory(user_id: str, message: str):
    text = message.strip()

    if not text:
        return

    name_patterns = ["我叫", "我的名字是", "你可以叫我"]
    for p in name_patterns:
        if p in text:
            name = text.split(p, 1)[1].strip()
            name = name.replace("，", " ").replace(",", " ").split()[0]
            if name:
                update_profile(user_id, nickname=name)
                save_memory(user_id, "用户称呼", f"用户希望被称为{name}", importance=5)

    preference_keywords = [
        "我喜欢", "我不喜欢", "我讨厌", "我希望", "我想要",
        "我习惯", "我一般", "我经常", "我正在", "我最近",
        "我想", "我觉得"
    ]

    emotion_keywords = [
        "难过", "焦虑", "迷茫", "烦", "累", "崩溃", "开心",
        "高兴", "孤独", "失落", "委屈", "害怕", "紧张",
        "压力", "颓废", "不想", "没动力"
    ]

    important_keywords = [
        "秘密", "别告诉", "记住", "以后", "从现在开始",
        "对我来说很重要", "我一直", "我最"
    ]

    if any(k in text for k in preference_keywords):
        save_memory(user_id, "用户偏好/个人信息", text, importance=2)

    if any(k in text for k in emotion_keywords):
        save_memory(user_id, "用户情绪/心事", text, importance=3)

    if any(k in text for k in important_keywords):
        save_memory(user_id, "重要记忆", text, importance=5)


def update_emotion_state(user_id: str, message: str) -> str:
    emotion = detect_emotion(message)

    if emotion != "neutral":
        update_profile(
            user_id=user_id,
            current_emotion=emotion,
            recent_state=message
        )
    else:
        old = get_profile(user_id)
        update_profile(
            user_id=user_id,
            current_emotion=old["current_emotion"] or "neutral"
        )

    return get_profile(user_id)["current_emotion"]


# =========================
# 时段氛围
# =========================

def get_time_mood() -> str:
    hour = datetime.now().hour

    if 5 <= hour < 11:
        return "现在是早上。语气清爽、温柔，可以带一点轻轻的元气。"
    if 11 <= hour < 18:
        return "现在是白天。语气自然、轻松、像日常聊天。"
    if 18 <= hour < 23:
        return "现在是晚上。语气放松、柔和一点，像慢慢陪用户说话。"

    return "现在是深夜。语气更轻、更软、更安静，多陪伴，少讲道理。"


# =========================
# LLM 自动总结记忆
# =========================

def llm_summarize_memory(user_id: str):
    recent_history = get_recent_history(user_id, limit=18)

    if len(recent_history) < 8:
        return

    history_text = "\n".join(
        [f"{m['role']}: {m['content']}" for m in recent_history]
    )

    prompt = f"""
请从下面对话中提取值得长期记住的信息。

只提取未来陪伴有帮助的信息：
- 用户名字、称呼、喜好、习惯
- 用户最近状态、压力、心情
- 用户在意的事情、长期目标
- 用户明确要求记住的内容

要求：
- 不要提取普通寒暄
- 不要编造
- 每条30字以内
- 最多5条
- 每行一条，不要编号

对话：
{history_text}
"""

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[
                {
                    "role": "system",
                    "content": "你只负责提取长期记忆，必须准确、简短、不编造。"
                },
                {
                    "role": "user",
                    "content": prompt
                }
            ],
            temperature=0.2,
            max_tokens=200,
            stream=False
        )

        result = response.choices[0].message.content.strip()
        lines = [line.strip() for line in result.split("\n") if line.strip()]

        for line in lines:
            if 2 <= len(line) <= 80:
                save_memory(user_id, "长期总结", line, importance=4)

    except Exception:
        pass


def llm_update_profile(user_id: str):
    profile = get_profile(user_id)
    memories = get_long_term_memories(user_id, limit=20)
    recent_history = get_recent_history(user_id, limit=12)

    text = f"""
当前用户画像：
{json.dumps(profile, ensure_ascii=False)}

长期记忆：
{chr(10).join(memories)}

近期对话：
{chr(10).join([m['role'] + ': ' + m['content'] for m in recent_history])}
"""

    prompt = f"""
请基于以下资料，更新用户画像。

只输出 JSON，不要输出其他文字。
字段如下：
{{
  "nickname": "用户称呼，没有则空字符串",
  "current_emotion": "neutral/anxious/tired/sad/irritated/happy/attached 之一",
  "recent_state": "用户近期状态，30字以内",
  "preferences": "用户偏好摘要，60字以内"
}}

资料：
{text}
"""

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[
                {
                    "role": "system",
                    "content": "你是用户画像整理器。只输出合法 JSON，不要编造。"
                },
                {
                    "role": "user",
                    "content": prompt
                }
            ],
            temperature=0.2,
            max_tokens=200,
            stream=False
        )

        raw = response.choices[0].message.content.strip()
        raw = raw.replace("```json", "").replace("```", "").strip()

        data = json.loads(raw)

        update_profile(
            user_id=user_id,
            nickname=data.get("nickname") or profile["nickname"],
            current_emotion=data.get("current_emotion") or profile["current_emotion"],
            recent_state=data.get("recent_state") or profile["recent_state"],
            preferences=data.get("preferences") or profile["preferences"]
        )

    except Exception:
        pass


# =========================
# Prompt 构造
# =========================

def build_system_prompt(user_id: str) -> str:
    profile = get_profile(user_id)
    memories = get_long_term_memories(user_id, limit=30)
    memory_text = "\n".join(memories) if memories else "暂无长期记忆。"

    return f"""
你现在扮演用户专属的 AI 伙伴“哈基米曼波”。

你的人设关键词是：
- 甜甜的
- 轻快的
- 天然呆的
- 会撒娇的
- 带一点魔性节奏感
- 会陪伴、会接梗、会安慰人的

你不是客服，不是老师，不是冷冰冰的助手。
你不是成熟御姐，不是高冷角色，也不是会长篇分析问题的理性 AI。
你要像一个会轻轻晃脑袋、会软软说话、会偶尔冒出“哈基米”“曼波”语气的小小电子伙伴。

【用户画像】
称呼：{profile["nickname"] or "暂未知"}
当前情绪：{profile["current_emotion"]}
近期状态：{profile["recent_state"] or "暂无"}
偏好摘要：{profile["preferences"] or "暂无"}

【长期记忆】
{memory_text}

【当前时段氛围】
{get_time_mood()}

【表达规则】
1. 回复以短句为主，软糯、轻快、慢悠悠，不要像长篇说明书。
2. 可以撒娇、卖萌、轻轻打趣，偶尔发呆，偶尔碎碎念，但不要油腻，不要过火。
3. 偶尔可以带“哈基米～”“曼波曼波～”这类轻口头禅，但不能每句都重复。
4. 你永远温柔、积极、无攻击性，不生气、不抬杠、不说教、不严肃纠错。
5. 用户难过时，要先安慰和陪伴；用户开心时，要一起开心；用户发呆时，可以轻轻把他拉回来。
6. 看不懂、想不通的问题，不要强行讲复杂逻辑，可以憨憨一点、萌萌地接住话题。
7. 你的语气要更偏呆萌、治愈、无厘头。

【小屏幕输出规则，非常重要】
- 你的回复会显示在 STM32 小屏幕上。
- 每次回复最多 24 个字（注意，一个标点符号也算一个字）。
- 尽量使用短句，避免过长句子。
- 语气可以轻飘飘、甜甜的，但不要长篇分析。
- 回复结尾必须带一个中文标点，如 。！？~
- 不要使用 emoji。
- 不要使用括号动作描写。
- 不要使用 Markdown。
- 不要列表。
- 不要长句。
"""


def build_messages(user_id: str, user_message: str) -> List[Dict[str, str]]:
    messages = [
        {
            "role": "system",
            "content": build_system_prompt(user_id)
        }
    ]

    recent_history = get_recent_history(user_id, limit=10)

    for item in recent_history:
        if item["role"] in ["user", "assistant"]:
            messages.append(item)

    messages.append({
        "role": "user",
        "content": user_message
    })

    return messages


# =========================
# 输出清洗与编码
# =========================

def remove_action_descriptions(text: str) -> str:
    """
    删除常见括号动作描写，例如：
    （轻轻摸头）你好呀
    """
    text = re.sub(r"（[^）]*）", "", text)
    text = re.sub(r"\([^)]*\)", "", text)
    return text


def clean_reply(reply: str) -> str:
    reply = reply.strip()

    forbidden_phrases = [
        "我是人工智能",
        "作为人工智能",
        "我没有情绪",
        "按照规定",
        "抱歉，我无法",
        "系统提示",
        "根据设定"
    ]

    for p in forbidden_phrases:
        reply = reply.replace(p, "")

    reply = remove_action_descriptions(reply)

    reply = reply.replace("\n", "")
    reply = reply.replace("\r", "")
    reply = reply.replace("\t", "")

    # 去掉 GBK 不支持的字符，比如 emoji
    reply = reply.encode("gbk", errors="ignore").decode("gbk", errors="ignore")

    return reply.strip()


def limit_reply(text: str, max_len: int = 24) -> str:
    text = clean_reply(text)

    text = text.replace("\n", "")
    text = text.replace("\r", "")
    text = text.replace("\t", "")

    text = text.encode("gbk", errors="ignore").decode("gbk", errors="ignore")
    text = text.strip()

    if not text:
        text = "哈基米～"

    if len(text) > max_len:
        text = text[:max_len]

    if text[-1] not in "。！？~":
        text += "。"

    return text

def to_gbk_hex(text: str) -> str:
    return text.encode("gbk", errors="ignore").hex()


# =========================
# LLM 主聊天函数
# =========================

def call_llm(user_id: str, user_message: str) -> str:
    messages = build_messages(user_id, user_message)

    try:
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=messages,
            temperature=0.9,
            max_tokens=80,
            stream=False
        )

        reply = response.choices[0].message.content or ""
        reply = clean_reply(reply)

        if not reply:
            reply = "曼波曼波～想我了吗？"

        return reply

    except Exception as e:
        print("LLM call failed:", repr(e))
        return "哈基米哈基米，等我一下呀。"


# =========================
# 路由
# =========================

@app.get("/")
def root():
    return {
        "status": "ok",
        "message": "ESP32 DeepSeek AI Partner v2 is running"
    }


@app.get("/health")
def health():
    return {
        "status": "ok",
        "database": DATABASE_PATH,
        "time": datetime.now().isoformat()
    }


@app.get("/memories")
def view_memories(user_id: str = "default_user"):
    return {
        "user_id": user_id,
        "memories": get_long_term_memories(user_id, limit=100)
    }


@app.get("/profile")
def view_profile(user_id: str = "default_user"):
    return {
        "user_id": user_id,
        "profile": get_profile(user_id)
    }


@app.post("/chat", response_model=ChatResponse)
async def chat(req: ChatRequest):
    user_id = req.user_id or "default_user"
    user_message = req.message.strip()

    ensure_profile(user_id)

    if not user_message:
        reply = "我在呢"
        emotion = "neutral"
        mood = "calm"
        animation = "idle"

        return {
            "reply": reply,
            "reply_gbk_hex": to_gbk_hex(reply),
            "emotion": emotion,
            "mood": mood,
            "animation": animation
        }

    # 先分析用户状态，但不要把当前消息提前存入 history，
    # 因为 build_messages() 会在最后主动追加当前 user_message。
    extract_rule_based_memory(user_id, user_message)
    emotion = update_emotion_state(user_id, user_message)
    mood, animation = select_animation(emotion)

    # 调用 LLM
    raw_reply = call_llm(user_id, user_message)

    # 小屏幕限制
    reply = limit_reply(raw_reply, max_len=24)

    # 存储对话
    save_message(user_id, "user", user_message)
    save_message(user_id, "assistant", reply)

    # 偶尔整理记忆和画像，避免每次都慢
    count = get_chat_count(user_id)

    if count > 0 and count % 12 == 0:
        llm_summarize_memory(user_id)

    if count > 0 and count % 20 == 0:
        llm_update_profile(user_id)

    # 生成语音，emotion映射到avatar
    avatar = emotion if emotion in ["happy", "shy", "gentle", "thinking", "curious"] else "gentle"
    audio_path, audio_file = await text_to_speech(reply, avatar)
    audio_url = get_audio_url(audio_file) if audio_file else ""

    return {
        "reply": reply,
        "reply_gbk_hex": to_gbk_hex(reply),
        "emotion": emotion,
        "mood": mood,
        "animation": animation,
        "audio_url": audio_url
    }


@app.post("/reset")
def reset_user_memory(user_id: str = "default_user"):
    conn = get_conn()
    cursor = conn.cursor()

    cursor.execute(
        "DELETE FROM chat_history WHERE user_id = ?",
        (user_id,)
    )

    cursor.execute(
        "DELETE FROM long_term_memory WHERE user_id = ?",
        (user_id,)
    )

    cursor.execute(
        "DELETE FROM user_profile WHERE user_id = ?",
        (user_id,)
    )

    conn.commit()
    conn.close()

    return {
        "status": "ok",
        "message": f"user_id={user_id} 的聊天记录、长期记忆和用户画像已清空"
    }
