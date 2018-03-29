/**
 * Simple ESP8266 server compliant with Mozilla's proposed WoT API
 * 
 * Based on the HelloServer example
 */
 
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "......";
const char* password = "..........";

ESP8266WebServer server(80);

const int ledPin = LED_BUILTIN;
bool ledOn = true;

const String ledDescr = "{\n"
" \"name\": \"ESP8266 LED\",\n"
"  \"type\": \"onOffSwitch\",\n"
"  \"href\": \"/\",\n"
"  \"properties\": {\n"
"    \"on\": {\n"
"      \"type\": \"boolean\",\n"
"      \"href\": \"/properties/on\"\n"
"    }\n"
"  },\n"
"  \"actions\": {},\n"
"  \"events\": {}\n"
"}";
  

void handleThing() {
  server.sendHeader("Access-Control-Allow-Origin", "*", false);
  server.sendHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS", false);
  server.send(200, "application/json", ledDescr);
}

void handleOnGet() {
  String response = "{\"on\":";
  response += ledOn;
  response += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*", false);
  server.sendHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS", false);
  server.send(200, "application/json", response);
}

void handleOnPut() {
  StaticJsonBuffer<128> newBuffer;
  JsonObject& onValue = newBuffer.parseObject(server.arg("plain"));
  bool newOn = onValue["on"];
  if (ledOn != newOn) {
    ledOn = newOn;
    digitalWrite(ledPin, ledOn ? LOW : HIGH); // active low led
  }
  server.sendHeader("Access-Control-Allow-Origin", "*", false);
  server.sendHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS", false);
  server.send(200, "application/json", server.arg("plain"));
}

void setup(void){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledOn ? LOW : HIGH);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleThing);
  server.on("/properties/on", HTTP_GET, handleOnGet);
  server.on("/properties/on", HTTP_PUT, handleOnPut);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
