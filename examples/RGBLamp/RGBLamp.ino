// -*- mode: c++;  c-basic-offset: 2 -*-
/**
 * Simple server compliant with Mozilla's proposed WoT API
 * Originally based on the HelloServer example
 * Tested on ESP8266, ESP32, Arduino boards with WINC1500 modules (shields or
 * MKR1000)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

//TODO: Hardcode your wifi credentials here (and keep it private)
const char* ssid = "public";
const char* password = "";

/// Only used for monitoring, can be removed it's not part of our "thing"
#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13;  // manully configure LED pin
#endif

//for optional properties
//const char * valEnum[5] = {"RED", "GREEN", "BLACK", "white", nullptr};
//const char * valEnum[5] = {"#db4a4a", "#4adb58", "000000", "ffffff", nullptr};

WebThingAdapter* adapter;

const char* deviceTypes[] = {"Light", "OnOffSwitch", "ColorControl", nullptr};
ThingDevice device("dimmable-color-light", "Dimmable Color Light", deviceTypes);

ThingProperty deviceOn("on", "Whether the led is turned on", BOOLEAN, "OnOffProperty");
ThingProperty deviceBrightness("brightness", "The level of light from 0-100", NUMBER, "BrightnessProperty");
ThingProperty deviceColor("color", "The color of light in RGB", STRING, "ColorProperty");

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
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
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
  adapter = new WebThingAdapter("rgb-lamp", WiFi.localIP());

  device.addProperty(&deviceOn);
  ThingPropertyValue brightnessValue;
  brightnessValue.number = 100; // default brightness TODO
  deviceBrightness.setValue(brightnessValue);
  device.addProperty(&deviceBrightness);

  //optional properties
  //deviceColor.propertyEnum = valEnum;
  //deviceColor.readOnly = true;
  //deviceColor.unit = "HEX";

  ThingPropertyValue colorValue;
  colorValue.string = &lastColor; //default color is white
  deviceColor.setValue(colorValue);
  device.addProperty(&deviceColor);

  adapter->addDevice(&device);
  Serial.println("Starting HTTP server");
  adapter->begin();
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(device.id);
#ifdef analogWriteRange
  analogWriteRange(255);
#endif
}

void update(String* color, int const brightness) {
  if (!color) return;
  float dim = brightness/100.;
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

  adapter->update();

  bool on = deviceOn.getValue().boolean;
  int brightness = deviceBrightness.getValue().number;
  update(&lastColor, on ? brightness : 0);

  if (on != lastOn) {
    Serial.print(device.id);
    Serial.print(": on: ");
    Serial.print(on);
    Serial.print(", brightness: ");
    Serial.print(brightness);
    Serial.print(", color: ");
    Serial.println(lastColor);
  }
  lastOn = on;
}
