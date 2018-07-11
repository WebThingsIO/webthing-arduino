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

#if defined(LED_BUILTIN)
const int lampPin = LED_BUILTIN;
#else
const int lampPin = 13;  // manully configure LED pin
#endif

WebThingAdapter* adapter;

const char* lampTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice lamp("lamp", "My Lamp", lampTypes);

ThingProperty lampOn("on", "Whether the lamp is turned on", BOOLEAN, "OnOffProperty");
ThingProperty lampLevel("level", "The level of light from 0-100", NUMBER, "BrightnessProperty");

void setup(void){
  pinMode(lampPin, OUTPUT);
  digitalWrite(lampPin, HIGH);
  Serial.begin(115200);
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
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
  adapter = new WebThingAdapter("led-lamp", WiFi.localIP());

  lamp.addProperty(&lampOn);
  lamp.addProperty(&lampLevel);
  adapter->addDevice(&lamp);
  adapter->begin();
  Serial.println("HTTP server started");

#ifdef analogWriteRange
  analogWriteRange(255);
#endif
}

void loop(void){
  adapter->update();
  if (lampOn.getValue().boolean) {
    int level = map(lampLevel.getValue().number, 0, 100, 255, 0);
    analogWrite(lampPin, level);
  } else {
    analogWrite(lampPin, 255);
  }
}
