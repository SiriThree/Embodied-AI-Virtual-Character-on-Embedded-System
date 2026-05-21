#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

HardwareSerial STM32Serial(2);

/* ESP32 Serial2 引脚 */
#define STM32_RX_PIN 16   // ESP32 RX2，接 STM32 PA9 / USART1_TX
#define STM32_TX_PIN 17   // ESP32 TX2，接 STM32 PA10 / USART1_RX

const char* ssid = "Redmi K70 Ultra";
const char* password = "wkm4m5gpb6zgsmz";

const char* serverUrl = "http://10.129.215.6:8000/chat";


void connectWiFi() {
  Serial.println();
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retryCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retryCount++;

    if (retryCount > 40) {
      Serial.println();
      Serial.println("WiFi connection failed.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
}


/*
 * 清理 STM32 发来的字符串
 * 注意：这里不能再只保留 ASCII，否则中文会被删掉
 */
String cleanText(String input) {
  String output = "";

  for (int i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input.charAt(i);

    /*
     * 删除 ASCII 控制字符：
     * 0~31 是控制字符，例如 \0 \r \n
     * 127 是 DEL
     *
     * 保留：
     * 普通英文、数字、符号
     * 中文 UTF-8 字节，通常 >= 128
     */
    if ((c >= 32 && c != 127) || c >= 128) {
      output += (char)c;
    }
  }

  output.trim();
  return output;
}


/*
 * JSON 字符串转义
 * 用于把用户输入安全放进 JSON body
 */
String jsonEscape(String input) {
  String output = "";

  for (int i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input.charAt(i);

    if (c == '\"') {
      output += "\\\"";
    } else if (c == '\\') {
      output += "\\\\";
    } else if (c == '\b') {
      output += "\\b";
    } else if (c == '\f') {
      output += "\\f";
    } else if (c == '\n') {
      output += "\\n";
    } else if (c == '\r') {
      output += "\\r";
    } else if (c == '\t') {
      output += "\\t";
    } else if (c < 32 || c == 127) {
      /*
       * 其他控制字符直接丢弃，避免 FastAPI 报：
       * Invalid control character
       */
      continue;
    } else {
      output += (char)c;
    }
  }

  return output;
}


String utf8SafeLimit(String input, int maxBytes) {
  if (input.length() <= maxBytes) {
    return input;
  }

  int cut = maxBytes;

  /*
   * 如果截断位置落在 UTF-8 后续字节上：
   * 后续字节格式是 10xxxxxx，也就是 0x80~0xBF
   * 就往前退，直到退到一个字符边界
   */
  while (cut > 0) {
    uint8_t c = (uint8_t)input.charAt(cut);

    if ((c & 0xC0) != 0x80) {
      break;
    }

    cut--;
  }

  String output = input.substring(0, cut);
  output += "...";
  return output;
}

/*
 * 单次调用 FastAPI
 * 期望 FastAPI 返回格式：
 * {
 *   "reply": "你好，我是你的AI伙伴"
 * }
 */
String callLLMServerOnce(String userInput) {
  HTTPClient http;

  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String cleanInput = cleanText(userInput);
  String safeInput = jsonEscape(cleanInput);

  String jsonBody = "{\"message\":\"" + safeInput + "\"}";

  Serial.println();
  Serial.println("Sending message to FastAPI...");
  Serial.print("Clean input: ");
  Serial.println(cleanInput);
  Serial.print("Request body: ");
  Serial.println(jsonBody);

  int httpCode = http.POST(jsonBody);

  Serial.print("HTTP status code: ");
  Serial.println(httpCode);

  String payload = http.getString();
  http.end();

  Serial.println("Raw response:");
  Serial.println(payload);

  if (httpCode != 200) {
    return "HTTP_FAILED";
  }

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("JSON parse failed.");
    return "JSON_FAILED";
  }

  const char* replyGbkHex = doc["reply_gbk_hex"];

  if (replyGbkHex == nullptr) {
    Serial.println("NO reply_gbk_hex field.");
    return "NO_GBK_HEX";
  }

  return String(replyGbkHex);
}


/*
 * 带重连和重试的 LLM 调用
 */
String callLLMServer(String userInput) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectWiFi();

    if (WiFi.status() != WL_CONNECTED) {
      return "WiFi failed";
    }
  }

  for (int i = 1; i <= 3; i++) {
    Serial.print("HTTP try ");
    Serial.println(i);

    String result = callLLMServerOnce(userInput);

    if (result != "HTTP_FAILED" &&
        result != "JSON_FAILED" &&
        result != "NO_REPLY") {
      return result;
    }

    Serial.println("Request failed, retrying...");
    delay(1000);
  }

  return "Cloud request failed";
}

int hexCharToValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

void sendHexAsBytesToSTM32(String hexStr) {
  int len = hexStr.length();

  for (int i = 0; i + 1 < len; i += 2) {
    int high = hexCharToValue(hexStr.charAt(i));
    int low  = hexCharToValue(hexStr.charAt(i + 1));

    if (high < 0 || low < 0) {
      continue;
    }

    uint8_t b = (uint8_t)((high << 4) | low);
    STM32Serial.write(b);
  }

  STM32Serial.write('\n');
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  STM32Serial.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  Serial.println();
  Serial.println("========== ESP32 BOOT ==========");
  Serial.println("ESP32 STM32 LLM Bridge Starting...");

  connectWiFi();

  Serial.println();
  Serial.println("Waiting for message from STM32...");

  /* 可选：启动时告诉 STM32 ESP32 已启动 */
  STM32Serial.println("ESP32 BOOT OK");
}


void loop() {
  if (STM32Serial.available()) {
    String stm32Message = STM32Serial.readStringUntil('\n');
    stm32Message = cleanText(stm32Message);

    if (stm32Message.length() == 0) {
      Serial.println();
      Serial.println("Ignored empty STM32 message.");
      return;
    }

    Serial.println();
    Serial.print("STM32 says: ");
    Serial.println(stm32Message);

    String gbkHex = callLLMServer(stm32Message);

    if (gbkHex == "HTTP_FAILED" ||
        gbkHex == "JSON_FAILED" ||
        gbkHex == "NO_REPLY" ||
        gbkHex == "NO_GBK_HEX" ||
        gbkHex == "Cloud request failed" ||
        gbkHex == "WiFi failed") {
      STM32Serial.println("Cloud failed");
      Serial.println("AI reply failed.");
      return;
    }

    Serial.println("GBK hex reply:");
    Serial.println(gbkHex);

    sendHexAsBytesToSTM32(gbkHex);

    Serial.println("GBK reply sent back to STM32.");
  }
}