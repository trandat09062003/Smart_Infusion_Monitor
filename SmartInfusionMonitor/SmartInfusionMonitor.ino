// Smart Infusion Monitoring System
// ESP32 + ITR9606 (IR drop sensor) + HX711 (load cell) + OLED 128x64 + Buzzer
// Cloud integration: ThingsBoard (MQTT) – optional Blynk support

/*
   Project Overview:
   • Monitor IV infusion rate (drops per minute) using ITR9606 infrared sensor.
   • Measure remaining fluid weight via 5 kg Load‑cell + HX711.
   • Display real‑time data on OLED 128×64.
   • Sound audible alarm when fluid level falls below critical threshold.
   • Push telemetry to cloud (ThingsBoard) – optionally Blynk.
   • ESP32‑DevKit V1 as the MCU.
*/

// ------------------------------------------------------------
// 1. Library Includes
// ------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>
#include <WiFi.h>
#include <PubSubClient.h>          // MQTT for ThingsBoard
//#define ENABLE_BLYNK                // Uncomment to enable Blynk support
#ifdef ENABLE_BLYNK
#include <BlynkSimpleEsp32.h>
#endif

// ------------------------------------------------------------
// 2. Pin Definitions (match schematic)
// ------------------------------------------------------------
#define ITR9606_PIN      18   // IR drop sensor – external interrupt
#define HX711_DOUT_PIN   16   // HX711 data output
#define HX711_SCK_PIN    17   // HX711 clock
#define BUZZER_PIN       19   // Buzzer (via transistor)

// I2C defaults on ESP32: SDA = GPIO21, SCL = GPIO22 (OLED)
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET    -1   // No separate reset line

// ------------------------------------------------------------
// 3. Global Objects & Display States
// ------------------------------------------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
HX711 scale;

// OLED refresh rate
const unsigned long OLED_REFRESH_INTERVAL = 200; // Refresh OLED every 200ms

// ------------------------------------------------------------
// 4. Configuration Parameters
// ------------------------------------------------------------
// Load‑cell calibration – adjust after real‑world tare/calibration
float CALIBRATION_FACTOR = 420.0;   // <-- calibrate with known weight

// Medical constants
const int DROPS_PER_ML = 20;        // Standard drip factor
const float CRITICAL_WEIGHT = 50.0; // Alert when < 50 g remaining

// Wi‑Fi / Cloud credentials – replace with real values
const char* WIFI_SSID     = "VIETSET_TECH";
const char* WIFI_PASSWORD = "vs68686868";

// ThingsBoard MQTT settings
const char* TB_SERVER   = "mqtt.eu.thingsboard.cloud"; // Change to your server
const int   TB_PORT     = 1883;
const char* TB_TOKEN    = "g3n3vvr1pwkK7DEEtsIp"; // Device token
WiFiClient espClient;
PubSubClient mqttClient(espClient);

#ifdef ENABLE_BLYNK
// Blynk authentication token – replace with your token
char BLYNK_AUTH[] = "YOUR_BLYNK_AUTH_TOKEN";
#endif

// ------------------------------------------------------------
// 5. Runtime Variables
// ------------------------------------------------------------
volatile unsigned long dropCount = 0;          // Total drops counted
volatile unsigned long lastDropTime = 0;       // Time of last drop (ms)
float currentFlowRateDPM = 0.0;                // Drops per minute
float currentFlowRateMLH = 0.0;                // Milliliters per hour
unsigned long lastUpdateSec = 0;              // OLED/Serial update timer
unsigned long lastBuzzerAlertTime = 0;        // Buzzer debounce timer
bool isBuzzerActive = false;                  // Alarm state flag

// ------------------------------------------------------------
// 6. Helper Functions
// ------------------------------------------------------------
// Render all 4 parameters on a single clean page
void drawPage(float weight, float dpm, float mlh, unsigned long drops, bool alert) {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  // Viewport offsets for the 0.66" 64x48 OLED:
  // Maps physical columns 0-63 to GDDRAM columns 32-95
  // Maps physical rows 0-47 to GDDRAM rows 16-63
  const int x_offset = 32;
  const int y_offset = 16;
  const int x_margin = x_offset + 4; // Left margin of 4 pixels on physical glass
  
  // Row 1: Weight (WT)
  display.setCursor(x_margin, y_offset + 2);
  display.print(F("W: "));
  display.print(weight, 1);
  display.print(F("g"));
  
  // Row 2: Drip Rate (Rate)
  display.setCursor(x_margin, y_offset + 11);
  display.print(F("R: "));
  display.print((int)dpm);
  display.print(F("dpm"));
  
  // Row 3: Volume Rate (Vol)
  display.setCursor(x_margin, y_offset + 20);
  display.print(F("V: "));
  display.print((int)mlh);
  display.print(F("ml/h"));
  
  // Row 4: Total Drops (Drops)
  display.setCursor(x_margin, y_offset + 29);
  display.print(F("D: "));
  display.print(drops);

  // Footer status bar (always present at bottom of the page, text only)
  display.setCursor(x_margin, y_offset + 39);
  if (alert) {
    // Blink alert text
    if ((millis() / 300) % 2 == 0) {
      display.print(F("ALERT"));
    }
  } else {
    display.print(F("SAFE"));
  }
}

// Interrupt routine – called on each drop detection
void IRAM_ATTR countDropISR() {
  unsigned long now = millis();
  // Simple debounce: ignore drops <150 ms apart
  if (now - lastDropTime > 150) {
    dropCount++;
    unsigned long interval = now - lastDropTime;
    if (lastDropTime > 0 && interval > 0) {
      currentFlowRateDPM = 60000.0 / interval;              // DPM calculation
      currentFlowRateMLH = (currentFlowRateDPM * 60.0) / DROPS_PER_ML; // Convert to mL/h
    }
    lastDropTime = now;
  }
}

// Connect to Wi‑Fi (blocking until success)
void connectWiFi() {
  Serial.print(F("[WiFi] Connecting to ")); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(F("."));
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[WiFi] Connected! IP:"));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\n[WiFi] Failed to connect. Proceeding in offline mode."));
  }
}

// MQTT reconnect logic for ThingsBoard (non-blocking)
bool mqttReconnect() {
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();
  if (now - lastReconnectAttempt >= 5000) { // Try to connect every 5 seconds
    lastReconnectAttempt = now;
    Serial.print(F("[MQTT] Attempting connection... "));
    // Client ID can be any unique string – use MAC address
    String clientId = "ESP32-" + String(WiFi.macAddress());
    if (mqttClient.connect(clientId.c_str(), TB_TOKEN, NULL)) {
      Serial.println(F("connected"));
      return true;
    } else {
      Serial.print(F("failed, rc=")); Serial.print(mqttClient.state());
      Serial.println(F(", will try again in 5s"));
    }
  }
  return false;
}

// Publish telemetry payload to ThingsBoard
void publishTelemetry(float weight, float flowDPM, float flowMLH, unsigned long drops, bool alert) {
  // Build JSON payload – keep it lightweight
  String payload = "{";
  payload += "\"weight\":" + String(weight, 1) + ",";
  payload += "\"flow_dpm\":" + String(flowDPM, 1) + ",";
  payload += "\"flow_mlh\":" + String(flowMLH, 1) + ",";
  payload += "\"drops\":" + String(drops) + ",";
  payload += "\"alert\":" + String(alert ? "true" : "false");
  payload += "}";
  mqttClient.publish("v1/devices/me/telemetry", payload.c_str());
}

// ------------------------------------------------------------
// 7. Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("=== Smart Infusion Monitor Initialization ==="));

  // Buzzer initialization
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // IR drop sensor interrupt configuration
  pinMode(ITR9606_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ITR9606_PIN), countDropISR, RISING);

  // Initialize I2C with custom pins from schematic: SDA=21, SCL=22
  Wire.begin(21, 22);

  // OLED initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[ERROR] OLED not found"));
    // Audible error beeps
    for (int i = 0; i < 3; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(150); digitalWrite(BUZZER_PIN, LOW); delay(150); }
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("SMART INFUSION"));
    display.println(F("INITIALIZING..."));
    display.display();
  }

  // Load‑cell (HX711) init
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  delay(500);
  if (scale.is_ready()) {
    Serial.println(F("[OK] HX711 detected"));
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare(); // Zero the scale
    Serial.println(F("[OK] Tare completed"));
  } else {
    Serial.println(F("[ERROR] HX711 not detected"));
  }

  // Wi‑Fi & Cloud
  connectWiFi();
  mqttClient.setServer(TB_SERVER, TB_PORT);
#ifdef ENABLE_BLYNK
  Blynk.config(BLYNK_AUTH);
#endif

  // Short pause before main loop
  delay(1000);
  display.clearDisplay();
}

// ------------------------------------------------------------
// 8. Main Loop
// ------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // Keep MQTT connection alive (if Wi‑Fi is up)
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      mqttReconnect();
    } else {
      mqttClient.loop();
    }
  }

#ifdef ENABLE_BLYNK
  Blynk.run();
#endif

  // ----- Read Load‑cell (once per second) -----
  static float remainingWeight = 0.0;
  static unsigned long lastScaleRead = 0;
  if (now - lastScaleRead >= 1000) {
    lastScaleRead = now;
    if (scale.is_ready()) {
      remainingWeight = scale.get_units(3); // Average 3 readings (non-blocking enough)
      if (remainingWeight < 0) remainingWeight = 0.0;
    } else {
      Serial.println(F("[WARN] Load cell not ready"));
    }
  }

  // ----- Detect idle (no drops) -----
  if (now - lastDropTime > 8000) { // 8 s without a drop → stop flow
    currentFlowRateDPM = 0.0;
    currentFlowRateMLH = 0.0;
  }

  // ----- Alarm handling -----
  bool lowLevelAlert = false;
  if (remainingWeight > 5.0 && remainingWeight < CRITICAL_WEIGHT) {
    lowLevelAlert = true;
    if (now - lastBuzzerAlertTime > 3000) { // 3‑s beep cycle
      // Two short beeps
      digitalWrite(BUZZER_PIN, HIGH); delay(100);
      digitalWrite(BUZZER_PIN, LOW);  delay(100);
      digitalWrite(BUZZER_PIN, HIGH); delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      lastBuzzerAlertTime = now;
      isBuzzerActive = true;
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    isBuzzerActive = false;
  }

  // ----- Serial output (once per second) -----
  if (now - lastUpdateSec >= 1000) {
    Serial.print(F("Weight: ")); Serial.print(remainingWeight, 1); Serial.print(F(" g | "));
    Serial.print(F("Flow: ")); Serial.print(currentFlowRateDPM, 1); Serial.print(F(" DPM | "));
    Serial.print(F("Vol: ")); Serial.print(currentFlowRateMLH, 1); Serial.print(F(" ml/h | "));
    Serial.print(F("Drops: ")); Serial.println(dropCount);
    lastUpdateSec = now;
  }

  // ----- OLED display update (every 200ms for smooth status bar blinking) -----
  static unsigned long lastOledRefreshTime = 0;
  if (now - lastOledRefreshTime >= OLED_REFRESH_INTERVAL) {
    lastOledRefreshTime = now;
    display.clearDisplay();
    drawPage(remainingWeight, currentFlowRateDPM, currentFlowRateMLH, dropCount, isBuzzerActive);
    display.display();
  }

  // ----- Cloud telemetry (every 5 s) -----
  static unsigned long lastTelemetry = 0;
  if (now - lastTelemetry >= 5000 && WiFi.status() == WL_CONNECTED) {
    publishTelemetry(remainingWeight, currentFlowRateDPM, currentFlowRateMLH, dropCount, lowLevelAlert);
    lastTelemetry = now;
  }
}

/*
   End of SmartInfusionMonitor.ino
*/
