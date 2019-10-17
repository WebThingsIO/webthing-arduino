/*
  WiFi Web Server LED control via web of things (e.g., iot.mozilla.org gateway)
  based on WiFi101.h example "Provisioning_WiFiWebServer.ino"

 A simple web server that lets you control an LED via the web.
 This sketch will print the IP address of your WiFi device (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to turn on and off the onboard LED.

 You can also connect via the Things Gateway http-on-off-wifi-adapter.

 If the IP address of your shield is yourAddress:
 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * WiFi using Microchip (Atmel) WINC1500
 * LED attached to pin 1 (onboard LED) for SAMW25
 * LED attached to pin 6 for MKR1000

 created 25 Nov 2012
 by Tom Igoe

 updates: dh, kg 2018
 */

#include <Arduino.h>
#include <BME280.h>
#include <BME280I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi101.h>
#include <Thing.h>
#include <WebThingAdapter.h>

#ifndef PIN_STATE_HIGH
#define PIN_STATE_HIGH HIGH
#endif
#ifndef PIN_STATE_LOW
#define PIN_STATE_LOW LOW
#endif

WebThingAdapter* adapter;

const char* bme280Types[] = {"TemperatureSensor", nullptr};
ThingDevice weather("bme280", "BME280 Weather Sensor", bme280Types);
ThingProperty weatherTemp("temperature", "", NUMBER, "TemperatureProperty");
ThingProperty weatherHum("humidity", "", NUMBER, nullptr);
ThingProperty weatherPres("pressure", "", NUMBER, nullptr);

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   0x76 // I2C address. I2C specific.
);

BME280I2C bme(settings);

void readBME280Data() {
   float temp(NAN), hum(NAN), pres(NAN);
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);
   bme.read(pres, temp, hum, tempUnit, presUnit);

   ThingPropertyValue value;
   value.number = pres;
   weatherPres.setValue(value);
   value.number = temp;
   weatherTemp.setValue(value);
   value.number = hum;
   weatherHum.setValue(value);
}

void setup() {
  //Initialize serial:
  // Serial.begin(9600);

  // check for the presence of the shield:
  // Serial.print("Configuring WiFi shield/module...\n");
  if (WiFi.status() == WL_NO_SHIELD) {
    // Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // configure the LED pin for output mode
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  while(!bme.begin()) {
    // Serial.println("Could not find BME280I2C sensor!");
    delay(1000);
  }

  // Serial.println("Starting in provisioning mode...");
  // Start in provisioning mode:
  //  1) This will try to connect to a previously associated access point.
  //  2) If this fails, an access point named "wifi101-XXXX" will be created, where XXXX
  //     is the last 4 digits of the boards MAC address. Once you are connected to the access point,
  //     you can configure an SSID and password by visiting http://wifi101/
  WiFi.beginProvision();

  while (WiFi.status() != WL_CONNECTED) {
    // wait while not connected

    // blink the led to show an unconnected status
    digitalWrite(LED_BUILTIN, PIN_STATE_HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, PIN_STATE_LOW);
    delay(500);
  }

  // connected, make the LED stay on
  digitalWrite(LED_BUILTIN, PIN_STATE_HIGH);
  adapter = new WebThingAdapter("weathersensor", WiFi.localIP());

  weatherTemp.unit = "celsius";
  weather.addProperty(&weatherTemp);
  weather.addProperty(&weatherPres);
  weather.addProperty(&weatherHum);
  adapter->addDevice(&weather);
  adapter->begin();
}

void loop() {
  readBME280Data();
  adapter->update();
}
