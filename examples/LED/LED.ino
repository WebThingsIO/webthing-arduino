/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "";
const char *password = "";

#if defined(LED_BUILTIN)
const int lampPin = LED_BUILTIN;
#else
const int lampPin = 13; // manually configure LED pin
#endif

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

bool lastOn = true;

void setup(void) {
  pinMode(lampPin, OUTPUT);
  digitalWrite(lampPin, HIGH);
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

  lamp.description = "A web connected lamp";

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
  ThingPropertyValue initialOn = {.boolean = true};
  lampOn.setValue(initialOn);
  (void)lampOn.changedValueOrNull();

}

void loop(void) {
  adapter->update();
  bool on = lampOn.getValue().boolean;
  if (on) {
    digitalWrite(lampPin, HIGH);
  } else {
    digitalWrite(lampPin, LOW);
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

  if (state) {
    digitalWrite(lampPin, HIGH);
  } else {
    digitalWrite(lampPin, LOW);
  }

  ThingDataValue value = {.boolean = state};
  lampOn.setValue(value);
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("toggle", input, do_toggle, nullptr);
}
