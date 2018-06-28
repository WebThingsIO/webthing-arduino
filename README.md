webthing-arduino
================

A simple server for the ESP8266, the ESP32, or any WiFi101-compatible board
that implements Mozilla's proposed Web of Things API. The [LED
example](https://github.com/mozilla-iot/webthing-arduino/blob/master/examples/LED)
exposes an onOffSwitch named "Built-in LED" which controls the board's built-in
LED. The [LED Lamp
example](https://github.com/mozilla-iot/webthing-arduino/blob/master/examples/LEDLamp)
ups the ante by introducing a `level` property to expose a dimmableLight.

## Arduino

### ESP8266 or ESP32

To run on either of these boards, download the Arduino IDE and set it up for board-specific
development. These Adafruit guides explain [how to set up for an
ESP8266](https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide)
and [how to set up for an
ESP32](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide).

### WiFi101

Install the WiFi101 library from the Arduino library manager.

### Continuing onwards

Make sure to install the ArduinoJson library if you don't have it
installed already.

![ArduinoJson install process](https://github.com/mozilla-iot/webthing-arduino/raw/master/docs/arduinojson.png)

Next, download this library from the same library manager by searching for `webthing`.

![add zip library and LED example](https://github.com/mozilla-iot/webthing-arduino/raw/master/docs/add-library-open-example.png)

You should be able to upload the example sketch onto your board and use it as a
simple Web Thing. This Web Thing can be talked to using the WoT API or added to
the Mozilla IoT Gateway using the "Add Thing by URL" feature. Note that right
now WiFi101-based Things must be manually added through typing the full URL to
the Web Thing, e.g. `http://192.168.0.103/things/led`.

If you want to create a Web Thing from scratch, make sure to include both
"Thing.h" and "WebThingAdapter.h". You can then add Things and Properties to
your board using our proposed API.

## PlatformIO

Add the `webthing-arduino` library through PlatformIO's package management
interface. You may also need to manually add the ArduinoJson and other
libraries to your project.

## Example

```c++
#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

const char* ssid = "......";
const char* password = "..........";

#if defined(LED_BUILTIN)
const int lampPin = LED_BUILTIN;
#else
const int lampPin = 13;  // manully configure LED pin
#endif

WebThingAdapter adapter("led-lamp");

const char* lampTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice lamp("lamp", "My Lamp", lampTypes);

ThingProperty lampOn("on", "Whether the lamp is turned on", BOOLEAN, "OnOffProperty");
ThingProperty lampLevel("level", "The level of light from 0-100", NUMBER, "BrightnessProperty");

void setup(void){
  pinMode(lampPin, OUTPUT);
  digitalWrite(lampPin, HIGH);
  Serial.begin(115200);
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

  lamp.addProperty(&lampOn);
  lamp.addProperty(&lampLevel);
  adapter.addDevice(&lamp);
  adapter.begin();
  Serial.println("HTTP server started");

  analogWriteRange(255);
}

void loop(void){
  adapter.update();
  if (lampOn.getValue().boolean) {
    int level = map(lampLevel.getValue().number, 0, 100, 255, 0);
    analogWrite(lampPin, level);
  } else {
    analogWrite(lampPin, 255);
  }
}
```
