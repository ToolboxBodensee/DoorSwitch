#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include "WebSocketClient.h"

#define STASSID "Toolbox"
#define STAPSK  ""
#define SPACE_TOKEN  
#define BUTTON_PIN D4
#define RESEND_COUNT 3
#define WS_DELAY 100

static constexpr int hysteresis_time_ms = 2000;
static constexpr int reed_interval_ms = 100;

#define API_STATE_URL "https://bodensee.space/cgi-bin/togglestate?space=toolbox-bodensee&token=" SPACE_TOKEN
static constexpr const char* api_state_open = API_STATE_URL "&state=open";
static constexpr const char* api_state_closed = API_STATE_URL "&state=closed";

struct led_control_t {
  bool secure;
  const char* url;
  uint16_t port;
  const char* path;
  const char* on_command;
  const char* off_command;
};

const led_control_t led_controls[] = {
  { false, "treppeled.intern.toolbox", 81, "/", "/12", "=off" },
  { false, "r42-led-tisch.intern.toolbox", 81, "/", "/11", "=off" },
  { false, "r42-led-wand.intern.toolbox", 81, "/", "/11", "=off" }
};

void setDoorState(bool doorState);

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  Serial.printf("\n\nConnecting to %s\n", STASSID);

  WiFi.hostname("DoorESP");
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("DoorESP");
  ArduinoOTA.onStart([]() {
    const char* type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.print("Start updating ");
    Serial.println(type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    const char* err_msg;
    switch(error)
    {
    case OTA_AUTH_ERROR: err_msg = "Auth Failed"; break;
    case OTA_BEGIN_ERROR: err_msg = "Begin Failed"; break;
    case OTA_CONNECT_ERROR: err_msg = "Connect Failed"; break;
    case OTA_RECEIVE_ERROR: err_msg = "Receive Failed"; break;
    case OTA_END_ERROR: err_msg = "End Failed"; break;
    default: err_msg = "Unknown"; break;
    }

    Serial.printf("Error[%u]: %s\n", error, err_msg);
  });
  ArduinoOTA.begin();
}

void loop() {
  static bool doorstate_curr = true;
  static uint32_t state_change_iter = 0;
  static constexpr uint32_t state_change_threshold = max(1, hysteresis_time_ms / reed_interval_ms);

  // check the current state of the switch
  bool doorstate_new = digitalRead(BUTTON_PIN) == HIGH;
  if (doorstate_curr != doorstate_new) {
    state_change_iter++;
  }else {
    // state is not consitent in change so start over again
    state_change_iter = 0;
  }

  // check if we need to propagate the change
  if (state_change_iter >= state_change_threshold) {
    state_change_iter = 0;
    // state long enought the same
    // so actual trigger the event
    doorstate_curr = doorstate_new;
    setDoorState(doorstate_curr);
  }

  delay(reed_interval_ms);
  ArduinoOTA.handle();
}

void sendLedWebsocketMessage(const led_control_t& led_control, const bool door_state) {
  WebSocketClient ws(led_control.secure);//secure
  ws.connect(led_control.url, led_control.path, led_control.port);
  if (!ws.isConnected()) {
    Serial.println("no led connection...");
    return;
  }
  String msg;
  if (ws.getMessage(msg)) {
    // expect Connected
    Serial.println(msg);
  }
  ws.send("$");
  if (ws.getMessage(msg)) {
    // expect OK
    Serial.println(msg);
  }
  ws.send(door_state ? led_control.on_command : led_control.off_command);
  if (ws.getMessage(msg)) {
    // expect OK
    Serial.println(msg);
  }
  delay(WS_DELAY);
  ws.disconnect();
}

void setDoorState(bool doorState) {
  Serial.printf("Setting door open to %d", doorState);

  String url(doorState ? api_state_open : api_state_closed);

  for (int i=0; i<RESEND_COUNT; i++) {
    BearSSL::WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    Serial.printf("HTTP response code: %u\n", http.GET());

    //quick hack led control
    for(const auto& led_control : led_controls) {
      sendLedWebsocketMessage(led_control, doorState);
    }
  }
}
