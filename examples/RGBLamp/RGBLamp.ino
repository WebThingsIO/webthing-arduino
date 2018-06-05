// -*- mode: c++;  c-basic-offset: 2 -*-
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
#include <Thing.h>
#include <WebThingAdapter.h>

//TODO: Hardcode your wifi credentials here (and keep it private)
const char* ssid = "public";
const char* password = "";

/// Only used for monitoring, can be removed it's not part of our "thing"
const int ledPin = LED_BUILTIN;

#ifdef ESP32
ESP32WebThingAdapter adapter("esp32");
#endif
#ifdef ESP8266
ESP8266WebThingAdapter adapter("esp8266");
#endif

ThingDevice device("dimmable-color-light", "Dimmable Color Light", "dimmableColorLight");

ThingProperty deviceOn("on", "Whether the led is turned on", BOOLEAN);
ThingProperty deviceLevel("level", "The level of light from 0-100", NUMBER);
ThingProperty deviceColor("color", "The color of light in RGB", STRING);

bool lastOn = false;
String lastColor = "#ffffff";

const unsigned char redPin = 12;
const unsigned char greenPin = 13;
const unsigned char bluePin = 14;


void setupLamp(void) {
  pinMode(redPin, OUTPUT);
  digitalWrite(redPin, HIGH);
  pinMode(greenPin, OUTPUT);
  digitalWrite(greenPin, HIGH);
  pinMode(bluePin, OUTPUT);
  digitalWrite(bluePin, HIGH);
}

void setup(void) {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  setupLamp();
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH); // active low led

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  device.addProperty(&deviceOn);
  ThingPropertyValue levelValue;
  levelValue.number = 100; // default brightness TODO
  deviceLevel.setValue(levelValue);
  device.addProperty(&deviceLevel);

  ThingPropertyValue colorValue;
  colorValue.string = &lastColor; //default color is white
  deviceColor.setValue(colorValue);
  device.addProperty(&deviceColor);

  adapter.addDevice(&device);
  Serial.println("Starting HTTP server");
  adapter.begin();
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(device.id);

  analogWriteRange(255);
}

void update(String* color, int const level) {
  if (!color) return;
  float dim = level/100.;
  int red,green,blue;
  if (color && (color->length() == 7) && color->charAt(0) == '#') {
    const char* hex = 1+(color->c_str()); // skip leading '#'
    sscanf(0+hex, "%2x", &red);
    sscanf(2+hex, "%2x", &green);
    sscanf(4+hex, "%2x", &blue);
  }
  analogWrite(redPin, red*dim);
  analogWrite(greenPin, green*dim );
  analogWrite(bluePin, blue*dim );
}

void loop(void) {
  digitalWrite(ledPin, micros() % 0xFFFF ? HIGH : LOW); // Heartbeat

  adapter.update();

  bool on = deviceOn.getValue().boolean;
  int level = deviceLevel.getValue().number;
  update(&lastColor, on ? level : 0);

  if (on != lastOn) {
    Serial.print(device.id);
    Serial.print(": on: ");
    Serial.print(on);
    Serial.print(", level: ");
    Serial.print(level);
    Serial.print(", color: ");
    Serial.println(lastColor);
  }
  lastOn = on;
}
