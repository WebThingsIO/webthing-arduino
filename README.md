# arduino-labthings

[![LabThings](https://img.shields.io/badge/-LabThings-8E00FF?style=flat&logo=data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4NCjwhRE9DVFlQRSBzdmcgIFBVQkxJQyAnLS8vVzNDLy9EVEQgU1ZHIDEuMS8vRU4nICAnaHR0cDovL3d3dy53My5vcmcvR3JhcGhpY3MvU1ZHLzEuMS9EVEQvc3ZnMTEuZHRkJz4NCjxzdmcgY2xpcC1ydWxlPSJldmVub2RkIiBmaWxsLXJ1bGU9ImV2ZW5vZGQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiIHN0cm9rZS1taXRlcmxpbWl0PSIyIiB2ZXJzaW9uPSIxLjEiIHZpZXdCb3g9IjAgMCAxNjMgMTYzIiB4bWw6c3BhY2U9InByZXNlcnZlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwYXRoIGQ9Im0xMjIuMjQgMTYyLjk5aDQwLjc0OHYtMTYyLjk5aC0xMDEuODd2NDAuNzQ4aDYxLjEyMnYxMjIuMjR6IiBmaWxsPSIjZmZmIi8+PHBhdGggZD0ibTAgMTIuMjI0di0xMi4yMjRoNDAuNzQ4djEyMi4yNGg2MS4xMjJ2NDAuNzQ4aC0xMDEuODd2LTEyLjIyNGgyMC4zNzR2LTguMTVoLTIwLjM3NHYtOC4xNDloOC4wMTl2LTguMTVoLTguMDE5di04LjE1aDIwLjM3NHYtOC4xNDloLTIwLjM3NHYtOC4xNWg4LjAxOXYtOC4xNWgtOC4wMTl2LTguMTQ5aDIwLjM3NHYtOC4xNWgtMjAuMzc0di04LjE0OWg4LjAxOXYtOC4xNWgtOC4wMTl2LTguMTVoMjAuMzc0di04LjE0OWgtMjAuMzc0di04LjE1aDguMDE5di04LjE0OWgtOC4wMTl2LTguMTVoMjAuMzc0di04LjE1aC0yMC4zNzR6IiBmaWxsPSIjZmZmIi8+PC9zdmc+DQo=)](https://github.com/labthings/)
[![Riot.im](https://img.shields.io/badge/chat-on%20riot.im-368BD6)](https://riot.im/app/#/room/#labthings:matrix.org)

A simple server for the ESP8266, the ESP32, boards with Ethernet, or any WiFi101-compatible board that implements the W3C Web of Things API.

## Installation

### PlatformIO

Add the `arduino-labthings` library through PlatformIO's package management interface. For example, in your projects `platformio.ini` file, include:

```ini
[global]
lib_deps =
    https://github.com/labthings/arduino-labthings

```

#### Using PlatformIO Core (CLI)

Run `platformio lib install` from within your project directory to install all dependencies,
and run `platformio run -e huzzah -t upload` to build and upload (replace `huzzah` with your boards environment).

See the `examples` folder for `platformio.ini` examples.

## Supported boards

### ESP8266 or ESP32 (Reccommended)

```ini
[platformio]
env_default= espressif32

[global]
lib_deps =
    https://github.com/labthings/arduino-labthings
monitor_speed = 115200

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    ${global.lib_deps}
    ESP Async WebServer
lib_ignore = WiFi101
lib_ldf_mode = deep+
monitor_speed =  ${global.monitor_speed}
```

### MKR1000, MKR1010, etc.

```ini
[platformio]
env_default= mkr1000USB

[global]
lib_deps =
    https://github.com/labthings/arduino-labthings
monitor_speed = 115200

[env:mkr1000USB]
platform = atmelsam
board = mkr1000USB
board_build.mcu = samd21g18a
framework = arduino
lib_deps =
    ${global.lib_deps}
    WiFi101
    ArduinoMDNS
lib_ldf_mode = deep+
monitor_speed =  ${global.monitor_speed}
```

### Seeed Wio Terminal

```ini
[platformio]
env_default= seeed_wio_terminal

[global]
lib_deps =
    https://github.com/labthings/arduino-labthings
monitor_speed = 115200

[env:seeed_wio_terminal]
platform = atmelsam
board = seeed_wio_terminal
framework = arduino
lib_deps =
    ${global.lib_deps}
    https://github.com/Seeed-Studio/Seeed_Arduino_atWiFi.git
    https://github.com/Seeed-Studio/Seeed_Arduino_FreeRTOS.git
    https://github.com/Seeed-Studio/esp-at-lib.git
    https://github.com/Seeed-Studio/Seeed_Arduino_mbedtls.git
    https://github.com/Seeed-Studio/Seeed_Arduino_atWiFiClientSecure.git
    https://github.com/Seeed-Studio/Seeed_Arduino_atUnified.git
    https://github.com/Seeed-Studio/Seeed_Arduino_atmDNS.git
    https://github.com/Seeed-Studio/Seeed_Arduino_atWebServer.git
    https://github.com/Seeed-Studio/Seeed_Arduino_FS.git
    https://github.com/Seeed-Studio/Seeed_Arduino_SFUD.git
lib_ldf_mode = deep+
monitor_speed =  ${global.monitor_speed}

```

### Ethernet board

If you want to create an ethernet LabThing, for example an Arduino Mega with an ethernet shield, make sure to include both "Thing.h" and "EthernetWebThingAdapter.h". 

```ini
[platformio]
env_default= ATmega2560

[global]
lib_deps =
    https://github.com/labthings/arduino-labthings
monitor_speed = 115200

[env:ATmega2560]
platform = atmelavr
board = ATmega2560
framework = arduino
lib_deps =
    ${global.lib_deps}
    ArduinoMDNS
    Ethernet
lib_ldf_mode = deep+
monitor_speed =  ${global.monitor_speed}
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
