#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>
#include <ESPmDNS.h>
#include <vector>
#include <ArduinoJson.h>

#define RST_PIN 27
#define SS_PIN 5

char ssid[32] = "defaultSSID";
char password[32] = "defaultPASSWORD";

const int ledPin = 2;

String ledState = "OFF";
String rfidState = "Key is not touching";

MFRC522 mfrc522(SS_PIN, RST_PIN);
AsyncWebServer server(80);

unsigned long lastCardPresentMillis = 0;
unsigned long cardTouchStartMillis = 0;
const long cardPresentInterval = 2000;

std::vector<String> openTimestamps;
std::vector<long> openDurations;

void connectToWiFi();
void saveConfig(String ssid, String pass);
void loadConfig();
String getLocalTime();

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  delay(100);
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  pinMode(ledPin, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  loadConfig();
  connectToWiFi();

  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/graf", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/graf.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/keyStatus.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/keyStatus.js", "application/javascript");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ssid = request->getParam("ssid", true)->value();
    String pass = request->getParam("pass", true)->value();
    saveConfig(ssid, pass);
    request->send(200, "text/html", "Settings saved! Restarting...");
    delay(2000);
    ESP.restart();
  });

  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    for (size_t i = 0; i < openTimestamps.size(); i++) {
      if (i > 0) json += ",";
      json += "{\"timestamp\":\"" + openTimestamps[i] + "\",\"state\":\"" + (i % 3 == 0 ? "Key is not touching" : (i % 3 == 1 ? "Key touching" : "Open lock")) + "\"}";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  server.on("/deleteNetwork", HTTP_GET, [](AsyncWebServerRequest *request) {
    SPIFFS.remove("/config.json");
    request->send(200, "text/plain", "Network settings deleted. Restarting...");
    delay(2000);
    ESP.restart();
  });

  server.on("/deleteDataLog", HTTP_GET, [](AsyncWebServerRequest *request) {
    openTimestamps.clear();
    openDurations.clear();
    request->send(200, "text/plain", "Data log deleted.");
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    SPIFFS.remove("/config.json");
    request->send(200, "text/plain", "Settings erased. Restarting...");
    delay(2000);
    ESP.restart();
  });

  configTime(0, 0, "pool.ntp.org");

  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (rfidState == "Key is not touching") {
      rfidState = "Key touching";
      cardTouchStartMillis = currentMillis;
      String timestamp = getLocalTime();
      Serial.println("Key touching at: " + timestamp);
    }
    lastCardPresentMillis = currentMillis;
    mfrc522.PICC_HaltA();
  } else if (currentMillis - lastCardPresentMillis > cardPresentInterval) {
    if (rfidState == "Key touching" || rfidState == "Open lock") {
      rfidState = "Key is not touching";
      if (!openDurations.empty()) {
        long duration = (currentMillis - cardTouchStartMillis) / 1000;
        openDurations.back() = duration;
      }
      Serial.println("Key is not touching");
    }
  }

  if (rfidState == "Key touching" && currentMillis - cardTouchStartMillis >= cardPresentInterval) {
    rfidState = "Open lock";
    String timestamp = getLocalTime();
    Serial.println("Open lock at: " + timestamp);
    openTimestamps.push_back(timestamp);
    openDurations.push_back(0);
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void saveConfig(String ssid, String pass) {
  DynamicJsonDocument doc(1024);
  doc["ssid"] = ssid;
  doc["password"] = pass;
  File configFile = SPIFFS.open("/config.json", FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void loadConfig() {
  File configFile = SPIFFS.open("/config.json", FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, configFile);
 
