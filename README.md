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
#include <Thing.h>
#include <WebThingAdapter.h>

WebThingAdapter adapter("localtoast");

ThingDevice toaster("toaster-1", "My Toaster", "toaster");

ThingProperty toasterOn("on", "Toasting", BOOLEAN);
ThingProperty toasterTimer("timer", "Time left in toast", NUMBER);
ThingProperty toasterToast("toast", "Toast requested", BOOLEAN);

ToasterInterface toasterInterface;

void setup() {
  toaster.addProperty(&toasterOn);
  toaster.addProperty(&toasterTimer);
  toaster.addProperty(&toasterToast);

  adapter.addDevice(&toaster);
}

void loop() {
  adapter.update();

  ThingPropertyValue onValue;
  onValue.boolean = toasterInterface.isToasting();
  toasterOn.setValue(onValue);

  ThingPropertyValue timerValue;
  timerValue.number = toasterInterface.getSecondsLeft();
  toasterTimer.setValue(timerValue);

  if (toasterToast.getValue().boolean) {
    ThingPropertyValue toastValue;
    toastValue.boolean = false;
    toasterToast.setValue(toastValue);
    toasterInterface.startToasting();
  }
}
```
