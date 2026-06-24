#include <WiFi.h>
#include <WebSocketsClient.h>
#include "BluetoothA2DPSource.h"
#include <esp_bt_main.h>
#include <esp_bt_device.h>

// -------- WIFI & VPS SETTINGS --------
const char* ssid = "xxxxx";
const char* password = "xxxxxx";
const char* vpsHost = "xxxxxx";  // Your VPS IP address
const int vpsPort = 8080;

// -------- BLUETOOTH SETUP --------
BluetoothA2DPSource a2dp_source;
bool alarmTriggered = false;
bool isBluetoothConnected = false;
bool isVpsConnected = false;

// -------- WEBSOCKET SETUP --------
WebSocketsClient webSocket;

// -------- MARIO THEME DATA --------
char notes[] = "EE E CE G g C g e a h bagEG AFG E CDh C g e a h bagEG AFG E CDh  GJFS E taC aCD GJFS E V VV  GJFS E taC aCD U D C  GJFS E taC aCD GJFS E V VV  GJFS E taC aCD U D C ";

int beats[] = {1,1,1,1,1,1,1,1,2,2,2,2,2,1,1,2,2,1,1,1,1,1,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,2,2,1,1,1,1,1,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,2,2,6,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,2,2,6};

int songLength = sizeof(notes) / sizeof(notes[0]);
int tempo = 100;

// Mario state
int currentNoteIndex = 0;
unsigned long noteStartTime = 0;
bool notePlaying = false;
int currentFreq = 0;
int currentDuration = 0;

// -------- FREQUENCY LOOKUP --------
int frequency(char note) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 't', 'a', 'b', 'h', 'C', 'D', 'S', 'E', 'F', 'J', 'G', 'A', 'V', 'U' };
  int frequencies[] = {262, 294, 330, 349, 392, 415, 440, 466, 494, 523, 587, 622, 659, 698, 740, 784, 880, 1047, 622};
  for (int i = 0; i < 19; i++) {
    if (names[i] == note) return frequencies[i];
  }
  return 0;
}

// -------- AUDIO GENERATOR (Mario Theme) --------
int32_t get_sound_data(Frame* frame, int32_t frameCount) {
  static int phase = 0;
  static int freq = 440;
  static unsigned long lastToggle = 0;

  // --- KEEP-ALIVE: Send a very low volume tone (almost inaudible) ---
  if (!alarmTriggered) {
    // Generate a very quiet 440Hz tone (volume = 2% of normal)
    static int keepAlivePhase = 0;
    for (int i = 0; i < frameCount; i++) {
      // Keep-alive with ultrasonic (19kHz - barely audible to humans)
      int16_t sample = 300 * sin(2 * 3.14159 * 19000 * i / 44100);  // 300 = very quiet
      frame[i].channel1 = sample;
      frame[i].channel2 = sample;
    }
    return frameCount;
  }

  // --- Alarm is ON - play the siren ---
  if (millis() - lastToggle > 200) {
    lastToggle = millis();
    freq = (freq == 440) ? 880 : 440;
  }

  for (int i = 0; i < frameCount; i++) {
    int16_t sample = 16000 * sin(2 * 3.14159 * freq * i / 44100);
    frame[i].channel1 = sample;
    frame[i].channel2 = sample;
  }
  return frameCount;
}

// -------- WEBSOCKET HANDLER --------
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  // In setup(), after webSocket setup:
 
  switch (type) {
    case WStype_CONNECTED:
      isVpsConnected = true;
      Serial.println(F("✅ VPS Connected"));
      break;
      
    case WStype_DISCONNECTED:
      isVpsConnected = false;
      Serial.println(F("❌ VPS Disconnected"));
      break;
      
    case WStype_TEXT:
      char* message = (char*)payload;
      Serial.print(F("📩 "));
      Serial.println(message);
      
      if (strcmp(message, "EMERGENCY_TRIGGERED") == 0) {
        alarmTriggered = true;
        currentNoteIndex = 0;  // Start from beginning
        notePlaying = false;   // Force first note to play
        Serial.println(F("🎵 MARIO THEME STARTED!"));
      }
      else if (strcmp(message, "EMERGENCY_CANCELLED") == 0) {
        alarmTriggered = false;
        Serial.println(F("🔕 ALARM OFF"));
      }
      break;
  }
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n🔄 Starting..."));

  Serial.print(F("📡 WiFi"));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F(" ✅"));

  Serial.print(F("📡 VPS..."));
  webSocket.begin(vpsHost, vpsPort, "/");
  webSocket.onEvent(webSocketEvent);
  // In setup(), after webSocket setup:
  webSocket.sendTXT("ESP32_CONNECTED");
  webSocket.setReconnectInterval(5000);

  Serial.print(F("🔊 Bluetooth..."));
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
  esp_bt_dev_set_device_name("ESP32_Audio");
  esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));

  delay(1000);
  
  a2dp_source.start("JBL Go 4", get_sound_data);
  
  Serial.println(F(" Looking for speaker..."));
  Serial.println(F("\n✅ Setup Complete. Waiting for commands."));
}

// -------- LOOP --------
void loop() {
  webSocket.loop();
  // In loop():
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {  // Every 30 seconds
    webSocket.sendTXT("HEARTBEAT");
    lastHeartbeat = millis();
  } 
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 10000) {
    lastStatus = millis();
    if (alarmTriggered) {
      Serial.println(F("🎵 Mario Theme Playing..."));
    } else {
      Serial.println(F("⏸️ Idle - connection kept alive"));
    }
  }
}