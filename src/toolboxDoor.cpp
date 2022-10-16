#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include "WebSocketClient.h"

#define STASSID "Toolbox"
#define STAPSK  ""
#define SPACE_TOKEN  
#define BUTTON_PIN D4

#define HYSTERESIS_TIME_MS  2000
#define REED_INERVAL_MS     100

#define API_STATE_URL       "https://bodensee.space/cgi-bin/togglestate?space=toolbox-bodensee&token=SPACE_TOKEN&state="
#define API_STATE_OPEN      "open"
#define API_STATE_CLOSED    "closed"

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

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(STASSID);

  WiFi.hostname("DoorESP");
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("DoorESP");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void loop() {
  static bool doorstate_curr = true;
  static uint32_t state_change_iter = 0;
  static uint32_t state_change_threshold = max(1, HYSTERESIS_TIME_MS / REED_INERVAL_MS);

  // check the current state of the switch
  bool doorstate_new = digitalRead(BUTTON_PIN) == HIGH;
  if (doorstate_curr != doorstate_new) {
    state_change_iter += 1;
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

  delay(REED_INERVAL_MS);
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
  ws.disconnect();
}

void setDoorState(bool doorState) {
  Serial.print("Setting door open to ");
  Serial.println(doorState);

  String url = String(API_STATE_URL);
  if (doorState) {
    url.concat(API_STATE_OPEN);
  } else {
    url.concat(API_STATE_CLOSED);
  }

  BearSSL::WiFiClientSecure client = BearSSL::WiFiClientSecure();
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  Serial.print("HTTP response code: ");
  Serial.println(httpCode);
  //quick hack led control
  for(const auto& led_control : led_controls) {
    sendLedWebsocketMessage(led_control, doorState);
  }
}
