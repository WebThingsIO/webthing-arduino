esp8266-wot
===========

A simple server for the ESP8266 that implements Mozilla's proposed Web of
Things API. Exposes an onOffSwitch named `led` which controls the board's
built-in LED.

This can be added to the Gateway using the "Add Thing by URL" feature.

To run on an ESP8266, download the Arduino IDE and set it up for ESP8266
development. Make sure to install the ArduinoJson library if you don't have it
installed already. You should be able to upload onto the ESP8266 and use it as
a simple Web Thing.
