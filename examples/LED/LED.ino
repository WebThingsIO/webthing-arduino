/**
 * Simple ESP8266 server compliant with Mozilla's proposed WoT API
 * Based on the HelloServer example
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Thing.h"
#include "WebThingAdapter.h"

const char* ssid = "......";
const char* password = "..........";

const int ledPin = LED_BUILTIN;

#ifdef ESP32
ESP32WebThingAdapter adapter("esp32");
#endif
#ifdef ESP8266
ESP8266WebThingAdapter adapter("esp8266");
#endif

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
