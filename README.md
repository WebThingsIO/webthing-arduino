[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/WebThingsIO/webthing-arduino)

webthing-arduino
================

A simple Web of Things Thing Description compatible library, for the ESP32. 
Available examples
- [BME280](/examples/BME280)
- [LED](/examples/LED)
- [OLED display](/examples/TextDisplay)

## Installation

1. Download and install [ArduinoJSON](https://arduinojson.org/v6/doc/installation/)
1. Download and install [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer/) 
1. Download the code as ZIP file `Code -> Download ZIP`
2. Install the library in the Arduino IDE `Sketch -> Include Library -> Add ZIP Library...`

### Start development

You should upload one of the example applications to your board. 
The Web of Things Thing Description is available under `/.well-known/wot-thing-description`. 
All available actions and properties are described in the Thing Description. 

If you want to create a Web Thing from scratch, make sure to include both
"Thing.h" and "WebThingAdapter.h".
You can then add Actions and Properties to your board.

**Properties**
```c++
WebThingAdapter *adapter;

ThingProperty weatherTemp("temperature", "", NUMBER, "TemperatureProperty");

const char *bme280Types[] = {"TemperatureSensor", nullptr};
ThingDevice weather("bme280", "BME280 Weather Sensor", bme280Types);

weather.addProperty(&weatherTemp);
adapter->addDevice(&weather);

```

**Actions**
```c++
WebThingAdapter *adapter;
ThingActionObject *action_generator(DynamicJsonDocument *);

StaticJsonDocument<256> oledInput;
JsonObject oledInputObj = oledInput.to<JsonObject>();
ThingAction displayAction("display", "Display text", "display a text on OLED",
                 "displayAction", &oledInputObj, action_generator);

const char *displayTypes[] = {"OLED Display", nullptr};
ThingDevice displayThing("oled", "OLED Display", displayTypes);

oledThing.addAction(&displayAction);
adapter->addDevice(&displayThing);

void do_display(const JsonVariant &input){
  println("Action request");
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("display", input, callback, nullptr);
}
```

## Example

```c++
#include <Arduino.h>
#include "Thing.h"
#include "WebThingAdapter.h"

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "public";
const char *password = "";

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13; // manually configure LED pin
#endif

WebThingAdapter *adapter;

const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);
ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty");

bool lastOn = false;

void setup(void) {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
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
  adapter = new WebThingAdapter("w25", WiFi.localIP());

  led.addProperty(&ledOn);
  adapter->addDevice(&led);
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(led.id);
}

void loop(void) {
  adapter->update();
  bool on = ledOn.getValue().boolean;
  digitalWrite(ledPin, on ? LOW : HIGH); // active low led
  if (on != lastOn) {
    Serial.print(led.id);
    Serial.print(": ");
    Serial.println(on);
  }
  lastOn = on;
}
```

## Configuration

* If you have a complex device with large thing descriptions, you may need to
  increase the size of the JSON buffers. The buffer sizes are configurable as
  such:

    ```cpp
    // By default, buffers are 256 bytes for small documents, 1024 for larger ones

    // To use a pre-defined set of larger JSON buffers (4x larger)
    #define LARGE_JSON_BUFFERS 1

    // Else, you can define your own size
    #define SMALL_JSON_DOCUMENT_SIZE <something>
    #define LARGE_JSON_DOCUMENT_SIZE <something>

    #include <Thing.h>
    #include <WebThingAdapter.h>
    ```

