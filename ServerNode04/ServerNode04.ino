
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ==========================================
// WIFI CONFIGURATION
// ==========================================

const char* ssid = "ranes";
const char* password = "1234567a";

WebServer server(80);

// ==========================================
// NODE DATA
// ==========================================

String node04Data = "{}";

unsigned long lastNode04Update = 0;
const unsigned long NODE_TIMEOUT = 15000;

// ==========================================
// SYSTEM CONTROLS
// ==========================================

bool systemArmed = true;
bool buzzerEnabled = true;
bool buzzerTestRequested = false;

// ==========================================
// DASHBOARD HTML
// ==========================================

const char DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Campus Safety System</title>

<style>
* { box-sizing: border-box; }

body {
  margin: 0;
  font-family: Arial, sans-serif;
  background: #f1f5f9;
  color: #172033;
}

header {
  background: #14213d;
  color: white;
  padding: 24px;
  text-align: center;
}

header h1 { margin: 0; }

header p {
  color: #cbd5e1;
  margin-bottom: 0;
}

.container {
  max-width: 1100px;
  margin: auto;
  padding: 20px;
}

h2 { margin-top: 28px; }

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 15px;
}

.card {
  background: white;
  padding: 20px;
  border-radius: 12px;
  box-shadow: 0 3px 12px rgba(0,0,0,.06);
}

.status {
  font-size: 20px;
  font-weight: bold;
  margin-top: 12px;
}

.online, .normal { color: #15803d; }
.offline, .alert { color: #dc2626; }

.sensor {
  display: flex;
  justify-content: space-between;
  gap: 10px;
  padding: 13px 0;
  border-bottom: 1px solid #e2e8f0;
}

.sensor:last-child { border-bottom: none; }

button {
  padding: 12px 16px;
  border: none;
  border-radius: 7px;
  cursor: pointer;
  font-size: 14px;
  font-weight: bold;
  margin: 5px 3px;
}

.arm { background: #15803d; color: white; }
.disarm { background: #dc2626; color: white; }
.buzzer { background: #2563eb; color: white; }
.test { background: #7c3aed; color: white; }

button:active { opacity: .7; }

#controlMessage {
  font-size: 13px;
  color: #475569;
  margin-top: 15px;
}

footer {
  text-align: center;
  padding: 25px;
  color: #64748b;
}
</style>
</head>

<body>

<header>
  <h1>CAMPUS SAFETY SYSTEM</h1>
  <p>ESP32 Central Monitoring Dashboard</p>
</header>

<div class="container">

<h2>Node Monitoring</h2>

<div class="grid">

  <div class="card">
    <h3>Node 01</h3>
    <p>Placeholder Node</p>
    <div class="status offline">OFFLINE</div>
  </div>

  <div class="card">
    <h3>Node 02</h3>
    <p>Placeholder Node</p>
    <div class="status offline">OFFLINE</div>
  </div>

  <div class="card">
    <h3>Node 03</h3>
    <p>Placeholder Node</p>
    <div class="status offline">OFFLINE</div>
  </div>

  <div class="card">
    <h3>Node 04</h3>
    <p>Entrance Gate</p>
    <div id="connection" class="status offline">OFFLINE</div>
  </div>

</div>

<h2>Entrance Gate Sensors</h2>

<div class="grid">

  <div class="card">
    <h3>Sensor Status</h3>

    <div class="sensor">
      <span>Reed Switch</span>
      <strong id="reed">--</strong>
    </div>

    <div class="sensor">
      <span>PIR Motion</span>
      <strong id="pir">--</strong>
    </div>

    <div class="sensor">
      <span>Vibration</span>
      <strong id="vibration">--</strong>
    </div>

    <div class="sensor">
      <span>System Armed</span>
      <strong id="armed">--</strong>
    </div>

    <div class="sensor">
      <span>Alarm Status</span>
      <strong id="alarm">--</strong>
    </div>

    <div class="sensor">
      <span>Last Sensor Update</span>
      <strong id="updated">--</strong>
    </div>
  </div>

  <div class="card">
    <h3>System Controls</h3>

    <p>Security System</p>

    <button class="arm" onclick="sendControl({armed:true})">
      ARM SYSTEM
    </button>

    <button class="disarm" onclick="sendControl({armed:false})">
      DISARM SYSTEM
    </button>

    <hr>

    <p>Buzzer Control</p>

    <button class="buzzer" onclick="sendControl({buzzer:true})">
      BUZZER ON
    </button>

    <button class="disarm" onclick="sendControl({buzzer:false})">
      BUZZER OFF
    </button>

    <hr>

    <p>Buzzer Test</p>

    <button class="test" onclick="sendControl({buzzer_test:true})">
      TEST BUZZER
    </button>

    <p id="controlMessage">Ready.</p>
  </div>

</div>
</div>

<footer>
Campus Safety System | Node 04 - Entrance Gate
</footer>

<script>

async function updateDashboard() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    const node = data.nodes.NODE_04;

    const connection = document.getElementById('connection');

    if (node.online) {
      connection.innerText = 'ONLINE';
      connection.className = 'status online';

      document.getElementById('reed').innerText =
        node.gate_open ? 'OPEN' : 'CLOSED';

      document.getElementById('pir').innerText =
        node.motion_detected ? 'MOTION DETECTED' : 'NO MOTION';

      document.getElementById('vibration').innerText =
        node.vibration_detected ? 'DETECTED' : 'NONE';

      document.getElementById('armed').innerText =
        node.armed ? 'YES' : 'NO';

      const alarm = document.getElementById('alarm');
      alarm.innerText = node.alarm ? 'ALERT!' : 'NORMAL';
      alarm.className = node.alarm ? 'alert' : 'normal';

      document.getElementById('updated').innerText =
        node.timestamp || 'Available';

    } else {
      connection.innerText = 'OFFLINE';
      connection.className = 'status offline';

      document.getElementById('reed').innerText = '--';
      document.getElementById('pir').innerText = '--';
      document.getElementById('vibration').innerText = '--';
      document.getElementById('armed').innerText = '--';
      document.getElementById('alarm').innerText = '--';
      document.getElementById('updated').innerText = '--';
    }

  } catch (error) {
    document.getElementById('connection').innerText = 'SERVER ERROR';
  }
}

async function sendControl(payload) {
  const message = document.getElementById('controlMessage');
  message.innerText = 'Sending command...';

  try {
    const response = await fetch('/api/control', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(payload)
    });

    const result = await response.json();

    message.innerText = result.success
      ? 'Command saved. Node 04 will receive it shortly.'
      : 'Command failed.';

  } catch (error) {
    message.innerText = 'Unable to contact server.';
  }
}

setInterval(updateDashboard, 1000);
updateDashboard();

</script>
</body>
</html>
)rawliteral";

// ==========================================
// DASHBOARD
// ==========================================

void handleRoot() {
  server.send_P(200, "text/html", DASHBOARD);
}

// ==========================================
// RECEIVE NODE 04 REPORT
// ==========================================

void handleReport() {

  if (!server.hasArg("plain")) {
    server.send(400, "application/json",
                "{\"success\":false}");
    return;
  }

  String body = server.arg("plain");

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json",
                "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  if (doc["node_id"] != "NODE_04") {
    server.send(400, "application/json",
                "{\"success\":false,\"message\":\"Unknown node\"}");
    return;
  }

  node04Data = body;
  lastNode04Update = millis();

  server.send(200, "application/json",
              "{\"success\":true}");
}

// ==========================================
// DASHBOARD STATUS API
// ==========================================

void handleStatus() {

  bool online =
    lastNode04Update != 0 &&
    millis() - lastNode04Update < NODE_TIMEOUT;

  JsonDocument result;

  JsonObject nodes = result["nodes"].to<JsonObject>();

  for (int i = 1; i <= 3; i++) {
    String id = "NODE_0" + String(i);
    JsonObject placeholder = nodes[id].to<JsonObject>();
    placeholder["online"] = false;
  }

  JsonObject node = nodes["NODE_04"].to<JsonObject>();
  node["online"] = online;

  if (online) {
    JsonDocument sensorData;
    deserializeJson(sensorData, node04Data);

    node["gate_open"] =
      sensorData["gate_open"] | false;

    node["motion_detected"] =
      sensorData["motion_detected"] | false;

    node["vibration_detected"] =
      sensorData["vibration_detected"] | false;

    node["armed"] =
      sensorData["armed"] | false;

    node["alarm"] =
      sensorData["alarm"] | false;

    node["timestamp"] =
      sensorData["timestamp"] | "Unknown";
  }

  String output;
  serializeJson(result, output);

  server.send(200, "application/json", output);
}

// ==========================================
// NODE COMMAND API
// ==========================================

void handleCommand() {

  JsonDocument result;

  result["armed"] = systemArmed;
  result["buzzer"] = buzzerEnabled;

  String output;
  serializeJson(result, output);

  server.send(200, "application/json", output);
}

// ==========================================
// DASHBOARD CONTROL API
// ==========================================

void handleControl() {

  if (!server.hasArg("plain")) {
    server.send(400, "application/json",
                "{\"success\":false}");
    return;
  }

  JsonDocument doc;

  DeserializationError error =
    deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json",
                "{\"success\":false}");
    return;
  }

  if (!doc["armed"].isNull()) {
    systemArmed = doc["armed"].as<bool>();
  }

  if (!doc["buzzer"].isNull()) {
    buzzerEnabled = doc["buzzer"].as<bool>();
  }

  if (!doc["buzzer_test"].isNull()) {
    // A one-shot test request is handled by Node 04.
    // It is returned as a command flag below.
    buzzerTestRequested = doc["buzzer_test"].as<bool>();
  }

  server.send(200, "application/json",
              "{\"success\":true}");
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("CAMPUS SAFETY SYSTEM SERVER");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("SERVER CONNECTED");

  Serial.print("SERVER IP ADDRESS: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/report", HTTP_POST, handleReport);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/command", HTTP_GET, handleCommand);
  server.on("/api/control", HTTP_POST, handleControl);

  server.begin();

  Serial.println("HTTP SERVER STARTED");
}

// ==========================================
// LOOP
// ==========================================

void loop() {
  server.handleClient();
}
