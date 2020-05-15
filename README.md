webthing-arduino
================

A simple server for the ESP8266, the ESP32, boards with Ethernet, or any
WiFi101-compatible board that implements Mozilla's proposed Web of Things API.
The [LED
example](https://github.com/mozilla-iot/webthing-arduino/blob/master/examples/LED)
exposes an OnOffSwitch named "Built-in LED" which controls the board's built-in
LED. The [LED Lamp
example](https://github.com/mozilla-iot/webthing-arduino/blob/master/examples/LEDLamp)
ups the ante by introducing a `level` property to expose a dimmable Light.

## Arduino

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

### Continuing onwards

Make sure to install the current release of the ArduinoJson library (6) if you
don't have it installed already.

![ArduinoJson install process](https://github.com/mozilla-iot/webthing-arduino/raw/master/docs/arduinojson.png)

Next, download this library from the same library manager by searching for
`webthing`.

![add zip library and LED example](https://github.com/mozilla-iot/webthing-arduino/raw/master/docs/add-library-open-example.png)

You should be able to upload the example sketch onto your board and use it as a
simple Web Thing. This Web Thing can be talked to using the WoT API or added to
the Mozilla WebThings Gateway using the "Add Thing by URL" feature. Note that
right now, WiFi101-based Things must be manually added by typing the full URL
to the Web Thing, e.g. `http://192.168.0.103/things/led`.

If you want to create a Web Thing from scratch, make sure to include both
"Thing.h" and "WebThingAdapter.h" (or "EthernetWebThingAdapter.h", if using an
Ethernet board). You can then add Things and Properties to your board using our
proposed API.

## PlatformIO

Add the `webthing-arduino` library through PlatformIO's package management
interface. You may also need to manually add the ArduinoJson and other
libraries to your project.

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
