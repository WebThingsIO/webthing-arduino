# arduino-labthings

[![LabThings](https://img.shields.io/badge/-LabThings-8E00FF?style=flat&logo=data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4NCjwhRE9DVFlQRSBzdmcgIFBVQkxJQyAnLS8vVzNDLy9EVEQgU1ZHIDEuMS8vRU4nICAnaHR0cDovL3d3dy53My5vcmcvR3JhcGhpY3MvU1ZHLzEuMS9EVEQvc3ZnMTEuZHRkJz4NCjxzdmcgY2xpcC1ydWxlPSJldmVub2RkIiBmaWxsLXJ1bGU9ImV2ZW5vZGQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiIHN0cm9rZS1taXRlcmxpbWl0PSIyIiB2ZXJzaW9uPSIxLjEiIHZpZXdCb3g9IjAgMCAxNjMgMTYzIiB4bWw6c3BhY2U9InByZXNlcnZlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwYXRoIGQ9Im0xMjIuMjQgMTYyLjk5aDQwLjc0OHYtMTYyLjk5aC0xMDEuODd2NDAuNzQ4aDYxLjEyMnYxMjIuMjR6IiBmaWxsPSIjZmZmIi8+PHBhdGggZD0ibTAgMTIuMjI0di0xMi4yMjRoNDAuNzQ4djEyMi4yNGg2MS4xMjJ2NDAuNzQ4aC0xMDEuODd2LTEyLjIyNGgyMC4zNzR2LTguMTVoLTIwLjM3NHYtOC4xNDloOC4wMTl2LTguMTVoLTguMDE5di04LjE1aDIwLjM3NHYtOC4xNDloLTIwLjM3NHYtOC4xNWg4LjAxOXYtOC4xNWgtOC4wMTl2LTguMTQ5aDIwLjM3NHYtOC4xNWgtMjAuMzc0di04LjE0OWg4LjAxOXYtOC4xNWgtOC4wMTl2LTguMTVoMjAuMzc0di04LjE0OWgtMjAuMzc0di04LjE1aDguMDE5di04LjE0OWgtOC4wMTl2LTguMTVoMjAuMzc0di04LjE1aC0yMC4zNzR6IiBmaWxsPSIjZmZmIi8+PC9zdmc+DQo=)](https://github.com/labthings/)
[![Riot.im](https://img.shields.io/badge/chat-on%20riot.im-368BD6)](https://riot.im/app/#/room/#labthings:matrix.org)

A simple server for the ESP8266, the ESP32, ~~boards with Ethernet, or any
WiFi101-compatible board~~ that implements the W3C Web of Things API.

## Supported boards

### ESP8266 or ESP32

To run on either of these boards, download the Arduino IDE and set it up for
board-specific development. These Adafruit guides explain [how to set up for an
ESP8266](https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide)
and [how to set up for an
ESP32](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide).
You will also need to download the [ESP Async
WebServer](https://github.com/me-no-dev/ESPAsyncWebServer/) library and unpack
it in your sketchbook's libraries folder.

### MKR1000, MKR1010, etc. (not yet implemented)

* MKR1000 (and similar): Install the WiFi101 library from the Arduino library
  manager.
* MKR1010 (and similar): Install the WiFiNINA library from the Arduino library
  manager.

### Ethernet board (not yet implemented)

If you want to create a Web Thing from scratch, make sure to include both
"Thing.h" and "WebThingAdapter.h" (or "EthernetWebThingAdapter.h", if using an
Ethernet board). You can then add Things and Properties to your board using our
proposed API.

## Installation

### PlatformIO

Add the `webthing-arduino` library through PlatformIO's package management
interface. For example (using an espressif8266 huzzah board), in your projects `platformio.ini` file, include:

```ini
[platformio]
env_default= huzzah

[global]
lib_deps =
    https://github.com/labthings/arduino-labthings
monitor_speed = 115200
build_type = debug

[env:huzzah]
platform = espressif8266
board = huzzah
framework = arduino
lib_deps =
    ${global.lib_deps}
    ESP Async WebServer
lib_ignore = WiFi101
lib_ldf_mode = deep+
monitor_speed =  ${global.monitor_speed}
```

Run `platformio lib install` from within your project directory to install all dependencies,
and run `platformio run -e huzzah -t upload` to build and upload (replace `huzzah` with your boards environment).

See the `examples` folder for `platformio.ini` examples.

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
