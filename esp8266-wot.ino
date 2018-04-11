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
#include "Thing.h"
#include "WebThingAdapter.h"

const char* ssid = "......";
const char* password = "..........";

const int ledPin = LED_BUILTIN;

WebThingAdapter adapter("esp8266");

ThingDevice led("led", "Built-in LED", "onOffSwitch");

ThingProperty ledOn("on", "", BOOLEAN);

void setup(void){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
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

  led.addProperty(&ledOn);
  adapter.addDevice(&led);
  adapter.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  adapter.update();
  digitalWrite(ledPin, ledOn.getValue().boolean ? LOW : HIGH); // active low led
}
