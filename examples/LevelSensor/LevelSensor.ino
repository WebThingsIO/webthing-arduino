// -*- mode: c++;  c-basic-offset: 2 -*-
/**
 * Simple server compliant with Mozilla's proposed WoT API
 * Based on the RGBLamp example
 * Tested on Arduino Mega with Ethernet Shield
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <Thing.h>
#include <EthernetWebThingAdapter.h>

const char* deviceTypes[] = {"MultiLevelSensor", "Sensor", nullptr};
ThingDevice device("AnalogSensorDevice", "Analog Sensor pluged in single pin", deviceTypes);
ThingProperty property("level", "Analog Input pin", NUMBER, "LevelProperty");
WebThingAdapter* adapter = NULL;

const int sensorPin = A0;

double lastValue = 0;

int setupNetwork() {
  Serial.println(__FUNCTION__);
  //TODO: update with actual MAC address
  uint8_t ETHERNET_MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  uint8_t error = Ethernet.begin(ETHERNET_MAC);
  if (error == 0)
  {
    printf("error: %s\n",__FUNCTION__);
    return -1;
  }
  return 0;
}


void setup(void) {
  Serial.begin(115200);
  Serial.println(__FUNCTION__);
  while (0 != setupNetwork()){
    delay(5000);
  }
  IPAddress ip = Ethernet.localIP();
  Serial.print("log: IP=");
  Serial.println(ip);
  delay(3000);
  adapter = new WebThingAdapter("analog-sensor", ip);
  device.addProperty(&property);
  adapter->addDevice(&device);
  Serial.println("Starting HTTP server");
  adapter->begin();
  Serial.print("http://");
  Serial.print(ip);
  Serial.print("/things/");
  Serial.println(device.id);
}

void loop(void) {
  const int threshold = 1;
  int value = analogRead(sensorPin);
  double percent = (double) 100. - (value/1204.*100.);
  if (abs(percent - lastValue) >= threshold) {
    Serial.print("log: Value: ");
    Serial.print(value);
    Serial.print(" = ");
    Serial.print(percent);
    Serial.println("%");
    ThingPropertyValue levelValue;
    levelValue.number = percent;
    property.setValue(levelValue);
    lastValue = percent;
  }
  adapter->update();
}
