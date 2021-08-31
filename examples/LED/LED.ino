/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include "Thing.h"
#include "WebThingAdapter.h"

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "";
const char *password = "";

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 19; // manually configure LED pin
#endif

const int externalPin = 19;

ThingActionObject *action_generator(DynamicJsonDocument *);

WebThingAdapter *adapter;

const char *lampTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice lamp("lamp123", "My Lamp", lampTypes);

ThingProperty lampOn("state", "Whether the lamp is turned on", BOOLEAN,
                     "OnOffProperty");

StaticJsonDocument<256> toggleInput;
JsonObject toggleInputObj = toggleInput.to<JsonObject>();
ThingAction toggle("toggle", "Toggle", "toggle the lamp on/off",
                 "ToggleAction", &toggleInputObj, action_generator);

void setup(void) {
  pinMode(ledPin, OUTPUT);
  pinMode(externalPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
  WiFi.begin(ssid, password);
  
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
  adapter = new WebThingAdapter("led-lamp", WiFi.localIP());

  lamp.description = "A web connected led";

  lampOn.title = "On/Off";
  lamp.addProperty(&lampOn);

  toggleInputObj["type"] = "object";
    JsonObject stateInputProperties =
      toggleInputObj.createNestedObject("properties");
  JsonObject stateInput =
      stateInputProperties.createNestedObject("state");
  stateInput["type"] = "boolean";
  lamp.addAction(&toggle);

  adapter->addDevice(&lamp);
  adapter->begin();

  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(lamp.id);

  // set initial values
  ThingPropertyValue initialOn = {.boolean = false};
  lampOn.setValue(initialOn);
  digitalWrite(externalPin, LOW);
  (void)lampOn.changedValueOrNull();

}
bool lastOn = false;
void loop(void) {
  adapter->update();
  bool on = lampOn.getValue().boolean;

  if (on) {
    digitalWrite(externalPin, HIGH);
  } else {
    digitalWrite(externalPin, LOW);
  }

  if (lastOn != on) {
    lastOn = on;
  }
}

void do_toggle(const JsonVariant &input) {
  Serial.println("toggle call");
  
  JsonObject inputObj = input.as<JsonObject>();
  bool state = inputObj["state"];

  Serial.print("state: ");
  Serial.println(state);

  ThingDataValue value = {.boolean = state};
  lampOn.setValue(value);
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("toggle", input, do_toggle, nullptr);
}
