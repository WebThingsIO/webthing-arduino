# LEVELSENSOR #


## USAGE: ##

Build using Arduino IDE or from command line using GNU make
(it will pull tools automagically):
```
cd webthing-arduino
make -C examples/LevelSensor upload monitor
# Or adjust to device's actually serial port:
make -C examples/LevelSensor upload monitor MONITOR_PORT=/dev/ttyUSB1
```

Observe log:
```
setup
setupNetwork
log: IP=192.168.1.225
Starting HTTP server
http://192.168.1.225/things/AnalogSensorDevice
log: Value: 360 = 70.10%
(...)
```

Check from outside:

```
curl http://192.168.1.225/
[ { 
    "@context" : "https://iot.mozilla.org/schemas",
    "@type" : [ 
        "MultiLevelSensor",
        "Sensor"
      ],
    "href" : "/things/AnalogSensorDevice",
    "name" : "Analog Sensor plugged in single pin",
    "properties" : { "level" : { 
            "@type" : "LevelProperty",
            "href" : "/things/AnalogSensorDevice/properties/level",
            "type" : "number"
          } }
  } ]

# Check property value
curl http://192.168.1.225/things/AnalogSensorDevice/properties/level
{"level":42.1337}
```

Then it can be added to gateway.

Note, you may need to change the hardcoded MAC address,
if you run this example on several devices of your LAN.


## DEMO: ##

[![web-of-things-agriculture-20180712rzr.webm](https://s-opensource.org/wp-content/uploads/2018/07/web-of-things-agriculture-20180712rzr.gif)](https://player.vimeo.com/video/279677314#web-of-things-agriculture-20180712rzr.webm "Video Demo")

This "Smart Agriculture Web Of Thing" demo was done using:

* Arduino mega256
* with W5100 shield
* and FC-28 soil moisture analog 

Of course this could be adapted to other devices or sensor
(feedback welcome or ask for support)


## RESOURCES: ##

* https://github.com/mozilla-iot/webthing-arduino/
* https://www.arduinolibraries.info/libraries/webthing-arduino
* https://iot.mozilla.org/
* https://discourse.mozilla.org/t/esp-adapter/26664/9
* https://discourse.mozilla.org/t/esp32-demo/28297
* https://s-opensource.org/author/philcovalsamsungcom/ 
* irc://irc.mozilla.org/#iot
