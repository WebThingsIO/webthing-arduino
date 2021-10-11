[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/WebThingsIO/webthing-arduino)

webthing-arduino
================

A simple Web of Things Thing Description compatible library, for the ESP32 mainly.
Available examples
- [BME280](/examples/BME280)
- [LED](/examples/LED)
- [OLED display](/examples/TextDisplay)

## Beginner Tutorials

If you need a more extensive explanation of the usage of this library, we have some step by step tutorials available with more context.

### WebThings Arduino + Node-WoT

[Node-WoT](https://github.com/eclipse/thingweb.node-wot) is a Javascript client library to consume Web of Things devices.

- [Show number of Github forks and stars on Matrix display](https://bind.systems/blog/web-of-things-github-forks-stars/)
- [Run speedtest and show results on an OLED display](https://bind.systems/blog/web-of-things-speedtest/)

### WebThings Arduino + Node-RED

[Node-RED](https://nodered.org/) is a drag and drop tool for connecting devices. It requires little Javascript programming knowledge.

- [Connect temperature sensor to OLED display](https://bind.systems/blog/web-of-things-node-red-temperature-oled/)

### WebThings Arduino

- [Blinking LED](https://bind.systems/blog/web-of-things-led/)
- [Matrix display](https://bind.systems/blog/web-of-things-arduino-matrix-display/)
- [OLED display](https://bind.systems/blog/web-of-things-arduino-oled/)

## Installation

### ESP8266 or ESP32

To run on either of these boards, download the Arduino IDE and set it up for
board-specific development. These Adafruit guides explain [how to set up for an
ESP8266](https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide)
and [how to set up for an
ESP32](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide).
You will also need to download the [ESP Async
WebServer](https://github.com/me-no-dev/ESPAsyncWebServer/) library and unpack
it in your sketchbook's libraries folder.

### MKR1000, MKR1010, etc.

* MKR1000 (and similar): Install the WiFi101 library from the Arduino library
  manager.
* MKR1010 (and similar): Install the WiFiNINA library from the Arduino library
  manager.

## PlatformIO

Add the `webthing-arduino` library through PlatformIO's package management
interface. Ensure that you get the latest release by examining the entries
in the version number dropdown list. It may be sorted counter-intuitively.
You may also need to manually add the ArduinoJson and other libraries to 
your project.

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

# Adding to Gateway

To add your web thing to the WebThings Gateway, install the "Web Thing" add-on and follow the instructions [here](https://github.com/WebThingsIO/thing-url-adapter#readme).
