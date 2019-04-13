#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>

#define STASSID "Toolbox"
#define STAPSK  ""
#define BUTTON_PIN D4

bool doorOpen = true;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  doorOpen = digitalRead(BUTTON_PIN);

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
  if (doorOpen != digitalRead(BUTTON_PIN)) {
    doorOpen = digitalRead(BUTTON_PIN);
    setDoorOpen(doorOpen);
    delay(100);
  }
  ArduinoOTA.handle();
}

void setDoorOpen(bool doorOpen) {
  Serial.print("Setting door open to ");
  Serial.println(doorOpen);
  HTTPClient http;
  BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure();
  client->setInsecure();
  String url = String("https://bodensee.space/cgi-bin/togglestate?space=toolbox&token=&state=");
  if (doorOpen) {
    url.concat("open");
  } else {
    url.concat("closed");
  }
  http.begin(dynamic_cast<WiFiClient&>(*client), url);
  int httpCode = http.GET();
  Serial.print("HTTP response code: ");
  Serial.println(httpCode);
}
