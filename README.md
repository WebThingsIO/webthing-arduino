webthing-esp8266
===========

A simple server for the ESP8266 that implements Mozilla's proposed Web of
Things API. The [LED
example](https://github.com/mozilla-iot/webthing-esp8266/blob/master/examples/LED)
exposes an onOffSwitch named "Built-in LED" which controls the board's built-in
LED.

To run on an ESP8266, download the Arduino IDE and set it up for ESP8266
development. Make sure to install the ArduinoJson library if you don't have it
installed already. Next, download this a zip of this library and add it using
your Arduino IDE's Sketch > Include Library > Add ZIP Library option. You
should be able to upload the example sketch onto your board and use it as a
simple Web Thing. This Web Thing can be talked to using the WoT API or added to
the Mozilla IoT Gateway using the "Add Thing by URL" feature.

If you want to create a Web Thing from scratch, make sure to include both
"Thing.h" and "WebThingAdapter.h". You can then add Things and Properties to
your board using our proposed API.

Example
-------

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
  if (toasterInterface.isToasting()) {
    toasterOn.setValue({true});
  } else {
    toasterOn.setValue({false});
  }

  toasterTimer.setValue({toasterInterface.getSecondsLeft()});

  if (toasterToast.getValue().boolean) {
    toasterToast.setValue({false});
    toasterInterface.startToasting();
  }
}
```
