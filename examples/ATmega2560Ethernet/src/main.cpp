#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <Ethernet.h>
#include "Thing.h"
#include "EthernetWebThingAdapter.h"

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13; // manually configure LED pin
#endif

WebThingAdapter *adapter;

// Create device
const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);

// Create properties
ThingProperty ledOn("on", "Mock LED on or off", BOOLEAN, "OnOffProperty");

// Initial values
bool lastOn = false;

void setup(void)
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);

  // Set initial values
  ThingPropertyValue ledTextValue;
  ledTextValue.string = &lastText;
  ledText.setValue(ledTextValue);

  ThingPropertyValue initialLedLevel;
  initialLedLevel.number = 500;
  ledLevel.setValue(initialLedLevel);

  // Set up device

  // Add properties
  led.addProperty(&ledOn);

  // Add devices and start
  adapter = new WebThingAdapter(&led, "w25", Ethernet.localIP());
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(Ethernet.localIP());
}

void loop(void)
{
  adapter->update();
  bool on = ledOn.getValue().boolean;
  digitalWrite(ledPin, on ? LOW : HIGH); // active low led
  if (on != lastOn)
  {
    Serial.print(led.id);
    Serial.print(": ");
    Serial.println(on);
  }
  lastOn = on;
}
