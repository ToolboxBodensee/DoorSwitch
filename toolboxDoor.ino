#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

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
}

void loop() {
  if (doorOpen != digitalRead(BUTTON_PIN)) {
    doorOpen = digitalRead(BUTTON_PIN);
    setDoorOpen(doorOpen);
  }
  delay(100);
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
