/**
 * Simple ESP8266 server compliant with Mozilla's proposed WoT API
 * Based on the HelloServer example
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

const char* ssid = "......";
const char* password = "..........";

const int lampPin = LED_BUILTIN;

WebThingAdapter adapter("led-lamp");

ThingDevice lamp("lamp", "My Lamp", "dimmableLight");

ThingProperty lampOn("on", "Whether the lamp is turned on", BOOLEAN);
ThingProperty lampLevel("level", "The level of light from 0-100", NUMBER);

void setup(void){
  pinMode(lampPin, OUTPUT);
  digitalWrite(lampPin, HIGH);
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

  lamp.addProperty(&lampOn);
  lamp.addProperty(&lampLevel);
  adapter.addDevice(&lamp);
  adapter.begin();
  Serial.println("HTTP server started");

  analogWriteRange(255);
}

void loop(void){
  adapter.update();
  if (lampOn.getValue().boolean) {
    int level = map(lampLevel.getValue().number, 0, 100, 255, 0);
    analogWrite(lampPin, level);
  } else {
    analogWrite(lampPin, 255);
  }
}
