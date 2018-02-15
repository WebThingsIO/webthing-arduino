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
"  \"href\": \"/things/led\",\n"
"  \"properties\": {\n"
"    \"on\": {\n"
"      \"type\": \"boolean\",\n"
"      \"href\": \"/things/led/properties/on\"\n"
"    }\n"
"  },\n"
"  \"actions\": {},\n"
"  \"events\": {}\n"
"}";
  

void handleThings() {
  server.send(200, "application/json", "[" + ledDescr + "]");
}

void handleThing() {
  server.send(200, "application/json", ledDescr);
}

void handleOnGet() {
  String response = "{\"on\":";
  response += ledOn;
  response += "}";
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
  server.send(200, "application/json", server.arg("plain"));
}

void handleNotFound(){
  String message = "Page Not Found\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
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

  server.on("/", handleThings);
  server.on("/things", handleThings);
  server.on("/things/led", handleThing);
  server.on("/things/led/properties/on", HTTP_GET, handleOnGet);
  server.on("/things/led/properties/on", HTTP_PUT, handleOnPut);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
