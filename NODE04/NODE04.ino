
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <time.h>

// ==========================================
// WIFI AND SERVER CONFIGURATION
// ==========================================

const char* ssid = "ranes";
const char* password = "1234567a";

// Change this to your ESP32 SERVER IP
const char* SERVER_URL = "http://192.168.137.177";

const char* REPORT_ENDPOINT = "/api/report";
const char* COMMAND_ENDPOINT = "/api/command?node_id=NODE_04";

// ==========================================
// NODE INFORMATION
// ==========================================

const char* NODE_ID = "NODE_04";
const char* NODE_NAME = "Entrance Gate";
const char* FIRMWARE_VERSION = "1.0.0";

// ==========================================
// PIN CONFIGURATION
// ==========================================

#define REED_PIN 27
#define PIR_PIN 25
#define VIBRATION_PIN 26

#define LED_PIN 4
#define BUZZER_PIN 18

// With INPUT_PULLUP and a simple two-wire reed switch:
// LOW  = switch closed (magnet detected)
// HIGH = switch open (magnet away)
#define REED_OPEN_LEVEL HIGH

// Most SW-420 digital modules are active HIGH when vibration
// is detected, but verify your module's output behavior.
#define VIBRATION_ACTIVE_LEVEL HIGH

// ==========================================
// TIMING
// ==========================================

const unsigned long SENSOR_INTERVAL = 500;
const unsigned long REPORT_INTERVAL = 3000;
const unsigned long COMMAND_INTERVAL = 5000;

unsigned long lastSensorRead = 0;
unsigned long lastReport = 0;
unsigned long lastCommandPoll = 0;

// ==========================================
// SENSOR VARIABLES
// ==========================================

bool gateOpen = false;
bool motionDetected = false;
bool vibrationDetected = false;

bool armed = true;
bool alarmActive = false;

bool pirReady = false;
unsigned long pirWarmupStart = 0;

bool serverConnected = false;

// ==========================================
// STORAGE
// ==========================================

Preferences preferences;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

void connectWiFi();
void readSensors();
void updateAlarm();
void updateOutputs();
void displayStatus();
void sendSensorData();
void pollServerCommands();
void saveOfflineData();
void replayOfflineData();
String getTimestamp();

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==================================");
  Serial.println("   NODE 04 - ENTRANCE GATE");
  Serial.println("==================================");

  // Configure pins
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Start Preferences
  preferences.begin("node04", false);
  armed = preferences.getBool("armed", true);

  // Start LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS initialization failed!");
  }

  // Start Wi-Fi
  connectWiFi();

  // Start OTA
  ArduinoOTA.setHostname("NODE_04_ENTRANCE_GATE");
  ArduinoOTA.begin();

  // Start time synchronization
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // PIR warm-up period
  pirWarmupStart = millis();

  Serial.println("PIR SENSOR WARMING UP...");
  Serial.println("Please wait approximately 60 seconds.");

  Serial.println("==================================");
}

// ==========================================
// WIFI CONNECTION
// ==========================================

void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 20000) {

    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {

    serverConnected = true;

    Serial.println();
    Serial.println("SERVER CONNECTED");
    Serial.print("Node IP: ");
    Serial.println(WiFi.localIP());

  } else {

    serverConnected = false;

    Serial.println();
    Serial.println("SERVER NOT CONNECTED");
  }
}

// ==========================================
// SENSOR READING
// ==========================================

void readSensors() {

  // Reed switch
  int reedValue = digitalRead(REED_PIN);
  gateOpen = (reedValue == REED_OPEN_LEVEL);

  // PIR warm-up
  if (millis() - pirWarmupStart >= 60000) {
    pirReady = true;
  }

  // PIR sensor
  if (pirReady) {
    motionDetected = (digitalRead(PIR_PIN) == HIGH);
  } else {
    motionDetected = false;
  }

  // SW-420 vibration sensor
  vibrationDetected =
    (digitalRead(VIBRATION_PIN) == VIBRATION_ACTIVE_LEVEL);
}

// ==========================================
// ALARM LOGIC
// ==========================================

void updateAlarm() {

  alarmActive =
    armed &&
    (gateOpen || motionDetected || vibrationDetected);
}

// ==========================================
// LED AND BUZZER
// ==========================================

void updateOutputs() {

  if (alarmActive) {

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

  } else {

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ==========================================
// SERIAL MONITOR DISPLAY
// ==========================================

void displayStatus() {

  Serial.println();
  Serial.println("==================================");
  Serial.println("       NODE 04 - ENTRANCE GATE");
  Serial.println("==================================");

  Serial.print("SERVER STATUS : ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("CONNECTED");
  } else {
    Serial.println("DISCONNECTED");
  }

  Serial.println("----------------------------------");

  Serial.print("REED SWITCH   : ");
  Serial.println(gateOpen ? "OPEN" : "CLOSED");

  Serial.print("PIR SENSOR    : ");

  if (!pirReady) {
    Serial.println("WARMING UP");
  } else {
    Serial.println(motionDetected ?
                   "MOTION DETECTED" : "NO MOTION");
  }

  Serial.print("VIBRATION     : ");
  Serial.println(vibrationDetected ?
                 "DETECTED" : "NONE");

  Serial.println("----------------------------------");

  Serial.print("SYSTEM ARMED  : ");
  Serial.println(armed ? "YES" : "NO");

  Serial.print("ALARM STATUS  : ");
  Serial.println(alarmActive ? "ALERT!" : "NORMAL");

  Serial.println("==================================");
}

// ==========================================
// TIMESTAMP
// ==========================================

String getTimestamp() {

  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 100)) {
    return "time_not_synced";
  }

  char buffer[30];

  strftime(buffer, sizeof(buffer),
           "%Y-%m-%dT%H:%M:%S", &timeInfo);

  return String(buffer);
}

// ==========================================
// SEND SENSOR DATA TO SERVER
// ==========================================

void sendSensorData() {

  if (WiFi.status() != WL_CONNECTED) {
    serverConnected = false;
    saveOfflineData();
    return;
  }

  HTTPClient http;

  String url = String(SERVER_URL) + REPORT_ENDPOINT;

  JsonDocument doc;

  doc["node_id"] = NODE_ID;
  doc["node_name"] = NODE_NAME;

  doc["gate_open"] = gateOpen;
  doc["motion_detected"] = motionDetected;
  doc["vibration_detected"] = vibrationDetected;

  doc["armed"] = armed;
  doc["alarm"] = alarmActive;

  doc["timestamp"] = getTimestamp();
  doc["firmware_version"] = FIRMWARE_VERSION;

  String json;
  serializeJson(doc, json);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(json);

  if (httpCode >= 200 && httpCode < 300) {
    serverConnected = true;
  } else {
    serverConnected = false;
    saveOfflineData();
  }

  http.end();
}

// ==========================================
// SERVER COMMAND POLLING
// ==========================================

void pollServerCommands() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;

  String url = String(SERVER_URL) + COMMAND_ENDPOINT;

  http.begin(url);

  int httpCode = http.GET();

  if (httpCode == 200) {

    String response = http.getString();

    JsonDocument doc;

    DeserializationError error =
      deserializeJson(doc, response);

    if (!error) {

      if (!doc["armed"].isNull()) {

        armed = doc["armed"].as<bool>();

        preferences.putBool("armed", armed);
      }

      if (!doc["buzzer"].isNull()) {

        bool buzzerEnabled =
          doc["buzzer"].as<bool>();

        if (!buzzerEnabled) {
          digitalWrite(BUZZER_PIN, LOW);
        } else {
          updateOutputs();
        }
      }
    }
  }

  http.end();
}

// ==========================================
// OFFLINE DATA STORAGE
// ==========================================

void saveOfflineData() {

  File file = LittleFS.open("/offline.txt", "a");

  if (!file) {
    return;
  }

  JsonDocument doc;

  doc["node_id"] = NODE_ID;
  doc["node_name"] = NODE_NAME;

  doc["gate_open"] = gateOpen;
  doc["motion_detected"] = motionDetected;
  doc["vibration_detected"] = vibrationDetected;

  doc["armed"] = armed;
  doc["alarm"] = alarmActive;

  doc["timestamp"] = getTimestamp();
  doc["firmware_version"] = FIRMWARE_VERSION;

  serializeJson(doc, file);
  file.println();

  file.close();
}

// ==========================================
// REPLAY OFFLINE DATA
// ==========================================

void replayOfflineData() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!LittleFS.exists("/offline.txt")) {
    return;
  }

  File file = LittleFS.open("/offline.txt", "r");

  if (!file) {
    return;
  }

  String url = String(SERVER_URL) + REPORT_ENDPOINT;

  HTTPClient http;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  bool allSent = true;

  while (file.available()) {

    String json = file.readStringUntil('\n');
    json.trim();

    if (json.length() == 0) {
      continue;
    }

    int httpCode = http.POST(json);

    if (httpCode < 200 || httpCode >= 300) {
      allSent = false;
      break;
    }
  }

  file.close();
  http.end();

  if (allSent) {
    LittleFS.remove("/offline.txt");
  }
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  ArduinoOTA.handle();

  unsigned long currentMillis = millis();

  // Reconnect Wi-Fi when disconnected
  if (WiFi.status() != WL_CONNECTED) {

    serverConnected = false;

    static unsigned long lastReconnect = 0;

    if (currentMillis - lastReconnect >= 10000) {

      lastReconnect = currentMillis;

      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  } else {
    serverConnected = true;
  }

  // Read sensors every 500 ms
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {

    lastSensorRead = currentMillis;

    readSensors();

    updateAlarm();

    updateOutputs();

    displayStatus();
  }

  // Report data every 3 seconds
  if (currentMillis - lastReport >= REPORT_INTERVAL) {

    lastReport = currentMillis;

    sendSensorData();
  }

  // Poll server commands every 5 seconds
  if (currentMillis - lastCommandPoll >= COMMAND_INTERVAL) {

    lastCommandPoll = currentMillis;

    pollServerCommands();
  }
}
