#include <WiFi.h>
#include <HTTPClient.h>
#include "Audio.h"
#include <ArduinoJson.h>

// Update these values before flashing the ESP32.
static const char *WIFI_SSID = "test";
static const char *WIFI_PASS = "12345678";
static const char *API_BASE_URL = "http://192.168.161.231:8000";
static const char *API_PATH = "/scene_story_serial";

// UART2 <-> STM32
static const int STM32_RX_PIN = 16;
static const int STM32_TX_PIN = 17;
static const uint32_t STM32_BAUD = 115200;

// I2S <-> MAX98357 (Audio amplifier)
static const int I2S_BCLK = 26;
static const int I2S_LRC = 25;
static const int I2S_DOUT = 22;

Audio audio;

static const int HISTORY_LIMIT = 8;
static const int SCENE_COUNT = 12;
static const size_t SERIAL_BUF_SIZE = 512;

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

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
  }

  return WiFi.status() == WL_CONNECTED;
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
  const char *audioUrl = respDoc["audio_url"];

  if (replyHex == nullptr || optionHexArray.isNull() || optionHexArray.size() < 3 || avatar == nullptr) {
    return false;
  }

  len = 0;
  appendAsciiBytes(buf, len, "TEXT=");
  appendHexBytes(buf, len, String(replyHex));
  appendAsciiBytes(buf, len, "|OPT1=");
  appendHexBytes(buf, len, String((const char *)optionHexArray[0]));
  appendAsciiBytes(buf, len, "|OPT2=");
  appendHexBytes(buf, len, String((const char *)optionHexArray[1]));
  appendAsciiBytes(buf, len, "|OPT3=");
  appendHexBytes(buf, len, String((const char *)optionHexArray[2]));
  appendAsciiBytes(buf, len, "|AVATAR=");
  appendAsciiBytes(buf, len, avatar);
  if (audioUrl != nullptr) {
    appendAsciiBytes(buf, len, "|AUDIO=");
    appendAsciiBytes(buf, len, audioUrl);
  }
  appendAsciiBytes(buf, len, "\n");
  return true;
}

static void buildFallbackPacket(uint8_t *buf, size_t &len, bool unknownScene) {
  const char *replyHex = unknownScene ? "ced2cfc8c5e3c4e3cbe6b1e3c1c4c1c4" : "ced2cfc8c5e3c4e3bcccd0f8c1c4d1bd";
  const char *opt1Hex = unknownScene ? "c4c7c4e3cfc8bfaabfdad1bd" : "cfc8bcccd0f8d5e2b8f6bbb0cce2";
  const char *opt2Hex = unknownScene ? "c4e3bdf1ccecd3d0b5e3bfc9b0ae" : "bbbbb8f6c7e1cbc9b5c4bbb0cce2";
  const char *opt3Hex = unknownScene ? "bbbbb8f6b3a1beb0cad4cad4" : "b5c8cdf8c2e7bbd6b8b4d4d9c1c4";

  len = 0;
  appendAsciiBytes(buf, len, "TEXT=");
  appendHexBytes(buf, len, String(replyHex));
  appendAsciiBytes(buf, len, "|OPT1=");
  appendHexBytes(buf, len, String(opt1Hex));
  appendAsciiBytes(buf, len, "|OPT2=");
  appendHexBytes(buf, len, String(opt2Hex));
  appendAsciiBytes(buf, len, "|OPT3=");
  appendHexBytes(buf, len, String(opt3Hex));
  appendAsciiBytes(buf, len, "|AVATAR=");
  appendAsciiBytes(buf, len, unknownScene ? "curious" : "gentle");
  appendAsciiBytes(buf, len, "\n");
}

static bool callStoryApi(const SceneProfile &scene, SceneState &state, uint8_t *serialBuf, size_t &serialLen, String &reply, String &audioUrl) {
  if (!ensureWiFi()) {
    return false;
  }

  HTTPClient http;
  String url = String(API_BASE_URL) + API_PATH;
  http.begin(url);
  http.addHeader(F("Content-Type"), F("application/json"));

  DynamicJsonDocument reqDoc(1024);
  reqDoc[F("scene_name")] = scene.name;
  reqDoc[F("scene_description")] = scene.description;
  reqDoc[F("ai_intro")] = scene.intro;
  reqDoc[F("user_id")] = "stm32_player";

  JsonArray history = reqDoc.createNestedArray(F("history"));
  for (int i = 0; i < state.historyCount; ++i) {
    history.add(state.history[i]);
  }

  String body;
  serializeJson(reqDoc, body);

  int status = http.POST(body);
  if (status != HTTP_CODE_OK) {
    Serial.print(F("HTTP error:"));
    Serial.println(status);
    http.end();
    return false;
  }

  DynamicJsonDocument respDoc(2048);
  String raw = http.getString();
  DeserializationError err = deserializeJson(respDoc, raw);
  http.end();
  if (err) {
    Serial.print(F("[bridge] json parse failed: "));
    Serial.println(err.c_str());
    return false;
  }

  reply = String((const char *)respDoc[F("reply")]);
  reply.trim();

  audioUrl = String((const char *)respDoc[F("audio_url")]);
  audioUrl.trim();

  state.lastOptionCount = 0;
  JsonArray options = respDoc[F("user_options")].as<JsonArray>();
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

static void handleFrame(const String &frame) {
  String sceneKey;
  String userChoice;
  int userIndex;
  if (!parseFrame(frame, sceneKey, userChoice, userIndex)) {
    return;
  }

  int sceneIndex = findSceneIndex(sceneKey);
  if (sceneIndex < 0) {
    Serial2.println(F("TEXT=我先陪你随便聊聊吧|OPT1=那你先开口呀|OPT2=你今天有点可爱|OPT3=换个场景试试|AVATAR=curious"));
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
  String reply;
  String audioUrl;
  if (!callStoryApi(kScenes[sceneIndex], gSceneStates[sceneIndex], serialBuf, serialLen, reply, audioUrl)) {
    Serial2.print(F("TEXT="));
    Serial2.print(kScenes[sceneIndex].name);
    Serial2.println(F("的气氛刚刚好|OPT1=先靠近一点聊|OPT2=故意逗你一下|OPT3=把剧情往下推|AVATAR=gentle"));
    return;
  }

  if (reply.length() > 0) {
    appendHistory(sceneIndex, "AI:" + reply);
  }

  if (audioUrl.length() > 0) {
    audio.connecttohost(audioUrl.c_str());
  }

  Serial2.write(serialBuf, serialLen);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  if (ensureWiFi()) {
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(9);
  }
}

void loop() {
  audio.loop();

  if (!Serial2.available()) {
    delay(10);
    return;
  }

  String frame = Serial2.readStringUntil('\n');
  frame.trim();
  if (frame.length() == 0) {
    return;
  }

  if (frame.startsWith("SET_VOL:")) {
    int vol = frame.substring(8).toInt();
    if (vol < 0) vol = 0;
    if (vol > 21) vol = 21;

    audio.setVolume(vol);
    
    Serial.printf("[bridge] Volume set to: %d\n", vol);
    return;
  }

  if (frame.startsWith("SCENE:") || frame == "AUDIO_STOP") {
      audio.stopSong();
    }

  handleFrame(frame);
}
