#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Update these values before flashing the ESP32.
static const char *WIFI_SSID = "Redmi K70 Ultra";
static const char *WIFI_PASS = "wkm4m5gpb6zgsmz";
static const char *API_BASE_URL = "http://10.129.215.6:8000";
static const char *API_PATH = "/scene_story_serial";

// UART2 <-> STM32
static const int STM32_RX_PIN = 16;
static const int STM32_TX_PIN = 17;
static const uint32_t STM32_BAUD = 115200;

static const int HISTORY_LIMIT = 8;
static const int SCENE_COUNT = 12;
static const size_t SERIAL_BUF_SIZE = 768;

struct SceneProfile {
  const char *key;
  const char *name;
  const char *description;
  const char *intro;
};

struct SceneState {
  String history[HISTORY_LIMIT];
  int historyCount;
  String lastOptions[3];
  int lastOptionCount;
};

static const SceneProfile kScenes[SCENE_COUNT] = {
  {"shopping", "逛街约会", "一起逛街挑衣服、看小物件，氛围轻松又带点暧昧。", "你看起来很适合今天这场约会，我已经想听你第一句会怎么说了。"},
  {"gaming", "一起开黑", "一起打游戏、互相调侃、并肩作战，节奏热闹。", "开局前先让我听听，你今天是想带飞我，还是等我夸你呀？"},
  {"cafe", "咖啡馆", "坐在咖啡馆里慢慢聊天，空气安静又有一点心动。", "咖啡都还没凉，我倒先开始期待你会聊什么了。"},
  {"library", "图书馆", "在图书馆轻声交流，像并肩学习时偷偷靠近的感觉。", "这里要轻一点说话，不过你可以把心事偷偷讲给我听。"},
  {"night", "晚安夜聊", "夜晚的聊天更柔软，适合说真心话和小情绪。", "夜色这么安静，你现在最想让我先听见什么？"},
  {"encourage", "鼓励模式", "在你低落或疲惫时给你温柔鼓励，也能带一点俏皮。", "如果你今天有点累，我可以先站到你这边。"},
  {"walk", "散步吹风", "一起慢慢散步，聊轻松话题，也能自然推进关系。", "风刚刚好，我们边走边聊，看看谁先把气氛变得暧昧一点。"},
  {"dinner", "一起吃饭", "边吃边聊生活和口味，像很自然的陪伴。", "先别急着点菜，我更想知道你今天会怎么哄我开心。"},
  {"movie", "电影时间", "刚看完电影或者准备看电影，适合聊感受和代入感。", "电影还没开始，我倒想先看你会不会偷偷剧透自己的心思。"},
  {"music", "音乐分享", "一起分享歌单、旋律和心情，适合有氛围感的互动。", "把耳机分我一边吧，我想听歌，也想听你。"},
  {"exam", "考前陪伴", "在紧张和不安时给陪伴和打气，也能顺势缓和心情。", "先别被压力追着跑，你可以先把最想抱怨的那句交给我。"},
  {"rest", "休息陪伴", "不用太热闹，安静陪伴也可以很有故事感。", "就算什么都不急着说，我也可以先陪你把这段安静坐满。"},
};

static SceneState gSceneStates[SCENE_COUNT];

static int findSceneIndex(const String &key) {
  for (int i = 0; i < SCENE_COUNT; ++i) {
    if (key.equals(kScenes[i].key)) {
      return i;
    }
  }
  return -1;
}

static void appendHistory(int sceneIndex, const String &line) {
  SceneState &state = gSceneStates[sceneIndex];
  if (state.historyCount < HISTORY_LIMIT) {
    state.history[state.historyCount++] = line;
    return;
  }

  for (int i = 1; i < HISTORY_LIMIT; ++i) {
    state.history[i - 1] = state.history[i];
  }
  state.history[HISTORY_LIMIT - 1] = line;
}

static void resetSceneHistory(int sceneIndex) {
  gSceneStates[sceneIndex].historyCount = 0;
  gSceneStates[sceneIndex].lastOptionCount = 0;
  for (int i = 0; i < HISTORY_LIMIT; ++i) {
    gSceneStates[sceneIndex].history[i] = "";
  }
  for (int i = 0; i < 3; ++i) {
    gSceneStates[sceneIndex].lastOptions[i] = "";
  }
}

static bool parseFrame(const String &frame, String &sceneKey, String &userChoice, int &userIndex) {
  if (!frame.startsWith("SCENE:")) {
    return false;
  }

  userIndex = -1;

  int split = frame.indexOf('|');
  if (split < 0) {
    sceneKey = frame.substring(6);
    userChoice = "";
  } else {
    sceneKey = frame.substring(6, split);
    if (frame.indexOf("|USER:") == split) {
      userChoice = frame.substring(split + 6);
    } else if (frame.indexOf("|IDX:") == split) {
      String idxText = frame.substring(split + 5);
      idxText.trim();
      if (idxText.length() > 0) {
        userIndex = idxText.toInt();
      }
      userChoice = "";
    } else {
      userChoice = "";
    }
  }

  sceneKey.trim();
  userChoice.trim();
  return sceneKey.length() > 0;
}

static bool ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.println("[bridge] connecting wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[bridge] wifi ok, ip=");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("[bridge] wifi failed");
  return false;
}

static uint8_t hexNibble(char ch) {
  if (ch >= '0' && ch <= '9') {
    return (uint8_t)(ch - '0');
  }
  if (ch >= 'a' && ch <= 'f') {
    return (uint8_t)(ch - 'a' + 10);
  }
  if (ch >= 'A' && ch <= 'F') {
    return (uint8_t)(ch - 'A' + 10);
  }
  return 0;
}

static void appendAsciiBytes(uint8_t *buf, size_t &len, const char *text) {
  while (*text != '\0' && len < SERIAL_BUF_SIZE) {
    buf[len++] = (uint8_t)(*text);
    ++text;
  }
}

static void appendHexBytes(uint8_t *buf, size_t &len, const String &hexText) {
  size_t i = 0;
  while (i + 1 < hexText.length() && len < SERIAL_BUF_SIZE) {
    uint8_t value = (uint8_t)((hexNibble(hexText[i]) << 4) | hexNibble(hexText[i + 1]));
    buf[len++] = value;
    i += 2;
  }
}

static bool buildSerialPacketFromHex(JsonDocument &respDoc, uint8_t *buf, size_t &len) {
  const char *replyHex = respDoc["reply_gbk_hex"];
  JsonArray optionHexArray = respDoc["options_gbk_hex"].as<JsonArray>();
  const char *avatar = respDoc["avatar"];

  if (replyHex == nullptr || optionHexArray.isNull() || optionHexArray.size() < 3 || avatar == nullptr) {
    return false;
  }

  String optHex[3];
  for (int i = 0; i < 3; ++i) {
    const char *item = optionHexArray[i];
    if (item == nullptr) {
      return false;
    }
    optHex[i] = String(item);
  }

  len = 0;
  appendAsciiBytes(buf, len, "TEXT=");
  appendHexBytes(buf, len, String(replyHex));
  appendAsciiBytes(buf, len, "|OPT1=");
  appendHexBytes(buf, len, optHex[0]);
  appendAsciiBytes(buf, len, "|OPT2=");
  appendHexBytes(buf, len, optHex[1]);
  appendAsciiBytes(buf, len, "|OPT3=");
  appendHexBytes(buf, len, optHex[2]);
  appendAsciiBytes(buf, len, "|AVATAR=");
  appendAsciiBytes(buf, len, avatar);
  appendAsciiBytes(buf, len, "\n");
  return true;
}

static bool callStoryApi(const SceneProfile &scene, SceneState &state, uint8_t *serialBuf, size_t &serialLen, String &reply) {
  if (!ensureWiFi()) {
    return false;
  }

  HTTPClient http;
  String url = String(API_BASE_URL) + API_PATH;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument reqDoc(2048);
  reqDoc["scene_name"] = scene.name;
  reqDoc["scene_description"] = scene.description;
  reqDoc["ai_intro"] = scene.intro;
  reqDoc["user_id"] = "stm32_player";

  JsonArray history = reqDoc.createNestedArray("history");
  for (int i = 0; i < state.historyCount; ++i) {
    history.add(state.history[i]);
  }

  String body;
  serializeJson(reqDoc, body);

  Serial.print("[bridge] POST ");
  Serial.println(url);
  Serial.print("[bridge] body=");
  Serial.println(body);

  int status = http.POST(body);
  if (status != HTTP_CODE_OK) {
    Serial.print("[bridge] http failed, status=");
    Serial.println(status);
    http.end();
    return false;
  }

  DynamicJsonDocument respDoc(4096);
  String raw = http.getString();
  Serial.print("[bridge] raw response=");
  Serial.println(raw);
  DeserializationError err = deserializeJson(respDoc, raw);
  http.end();
  if (err) {
    Serial.print("[bridge] json parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  reply = String((const char *)respDoc["reply"]);
  reply.trim();

  state.lastOptionCount = 0;
  JsonArray options = respDoc["user_options"].as<JsonArray>();
  if (!options.isNull()) {
    for (JsonVariant value : options) {
      if (state.lastOptionCount >= 3) {
        break;
      }
      state.lastOptions[state.lastOptionCount++] = String((const char *)value.as<const char *>());
    }
  }
  if (!buildSerialPacketFromHex(respDoc, serialBuf, serialLen)) {
    return false;
  }
  return serialLen > 0;
}

static String buildFallback(const String &sceneName) {
  return "TEXT=" + sceneName + "的气氛刚刚好|OPT1=先靠近一点聊|OPT2=故意逗你一下|OPT3=把剧情往下推|AVATAR=gentle";
}

static void handleFrame(const String &frame) {
  String sceneKey;
  String userChoice;
  int userIndex;
  if (!parseFrame(frame, sceneKey, userChoice, userIndex)) {
    Serial.print("[bridge] ignored frame=");
    Serial.println(frame);
    return;
  }

  Serial.print("[bridge] recv frame=");
  Serial.println(frame);

  int sceneIndex = findSceneIndex(sceneKey);
  if (sceneIndex < 0) {
    Serial.print("[bridge] unknown scene=");
    Serial.println(sceneKey);
    Serial2.println("TEXT=我先陪你随便聊聊吧|OPT1=那你先开口呀|OPT2=你今天有点可爱|OPT3=换个场景试试|AVATAR=curious");
    return;
  }

  if (userIndex >= 0 && userIndex < gSceneStates[sceneIndex].lastOptionCount) {
    appendHistory(sceneIndex, "USER:" + gSceneStates[sceneIndex].lastOptions[userIndex]);
  } else if (userChoice.length() > 0) {
    appendHistory(sceneIndex, "USER:" + userChoice);
  } else {
    resetSceneHistory(sceneIndex);
  }

  uint8_t serialBuf[SERIAL_BUF_SIZE];
  size_t serialLen = 0;
  String serialLine;
  String reply;
  if (!callStoryApi(kScenes[sceneIndex], gSceneStates[sceneIndex], serialBuf, serialLen, reply)) {
    Serial.println("[bridge] fallback serial line");
    serialLine = buildFallback(kScenes[sceneIndex].name);
    reply = "";
    Serial.print("[bridge] send serial=");
    Serial.println(serialLine);
    Serial2.println(serialLine);
    return;
  }

  if (reply.length() > 0) {
    appendHistory(sceneIndex, "AI:" + reply);
  }

  Serial.println("[bridge] send serial=<gbk packet>");
  Serial2.write(serialBuf, serialLen);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  Serial.println();
  Serial.println("[bridge] boot");
  Serial.print("[bridge] uart2 baud=");
  Serial.println(STM32_BAUD);
  Serial.print("[bridge] api=");
  Serial.print(API_BASE_URL);
  Serial.println(API_PATH);
  ensureWiFi();
}

void loop() {
  if (!Serial2.available()) {
    delay(10);
    return;
  }

  String frame = Serial2.readStringUntil('\n');
  frame.trim();
  if (frame.length() == 0) {
    return;
  }

  handleFrame(frame);
}
