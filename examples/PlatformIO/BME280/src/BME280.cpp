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
#include "WiFi101WebThingAdapter.h"

#define USE_MDNS_RESPONDER  0

#if USE_MDNS_RESPONDER
#include <WiFiMDNSResponder.h>
#else
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>
#endif

#ifndef PIN_STATE_HIGH
#define PIN_STATE_HIGH HIGH
#endif
#ifndef PIN_STATE_LOW
#define PIN_STATE_LOW LOW
#endif


char mdnsName[] = "wifi101-XXXXXX"; // the MDNS name that the board will respond to
                                    // after WiFi settings have been provisioned.
// The -XXXXXX will be replaced with the last 6 digits of the MAC address.
// The actual MDNS name will have '.local' after the name above, so
// "wifi101-123456" will be accessible on the MDNS name "wifi101-123456.local".

byte mac[6];

WiFi101AsyncServer server;

#if USE_MDNS_RESPONDER
// Create a MDNS responder to listen and respond to MDNS name requests.
WiFiMDNSResponder mdnsResponder;
#else
WiFiUDP udp;
MDNS mdns(udp);
#endif

// void printWiFiStatus();

ThingDevice weather("bme280", "BME280 Weather Sensor", "thing");
ThingProperty weatherTemp("temperature", "", NUMBER);
ThingProperty weatherHum("humidity", "", NUMBER);
ThingProperty weatherPres("pressure", "", NUMBER);

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

  WiFi.macAddress(mac);
  // Replace the XXXXXX in wifi101-XXXXXX with the last 6 digits of the MAC address.
  if (strcmp(&mdnsName[7], "-XXXXXX") == 0) {
    sprintf(&mdnsName[7], "-%.2X%.2X%.2X", mac[2], mac[1], mac[0]);
  }

  weather.addProperty(&weatherTemp);
  weather.addProperty(&weatherPres);
  weather.addProperty(&weatherHum);
  server.addDevice(&weather);
  server.begin();

#if USE_MDNS_RESPONDER
  // Setup the MDNS responder to listen to the configured name.
  // NOTE: You _must_ call this _after_ connecting to the WiFi network and
  // being assigned an IP address.
  if (!mdnsResponder.begin(mdnsName)) {
    // Serial.println("Failed to start MDNS responder!");
    while(1);
  }
#else
 // Initialize the mDNS library. You can now reach or ping this
  // Arduino via the host name "arduino.local", provided that your operating
  // system is mDNS/Bonjour-enabled (such as MacOS X).
  // Always call this before any other method!
  mdns.begin(WiFi.localIP(), mdnsName);

  // Now let's register the service we're offering (a web service) via Bonjour!
  // To do so, we call the addServiceRecord() method. The first argument is the
  // name of our service instance and its type, separated by a dot. In this
  // case, the service type is _http. There are many other service types, use
  // google to look up some common ones, but you can also invent your own
  // service type, like _mycoolservice - As long as your clients know what to
  // look for, you're good to go.
  // The second argument is the port on which the service is running. This is
  // port 80 here, the standard HTTP port.
  // The last argument is the protocol type of the service, either TCP or UDP.
  // Of course, our service is a TCP service.
  // With the service registered, it will show up in a mDNS/Bonjour-enabled web
  // browser. As an example, if you are using Apple's Safari, you will now see
  // the service under Bookmarks -> Bonjour (Provided that you have enabled
  // Bonjour in the "Bookmarks" preferences in Safari).
  mdns.addServiceRecord("http-on-off._http",
                        80,
                        MDNSServiceTCP);
#endif

  // Serial.print("Server listening at http://");
  // Serial.print(mdnsName);
  // Serial.println(".local/");

  // you're connected now, so print out the status:
  // printWiFiStatus();
}

unsigned long lastPrint = 0;

void loop() {
#if USE_MDNS_RESPONDER
  // Call the update() function on the MDNS responder every loop iteration to
  // make sure it can detect and respond to name requests.
  mdnsResponder.poll();
#else
  mdns.run();
#endif

  // print wifi status every 30 seconds
  unsigned long now = millis();
  if ((now - lastPrint) > 30000) {
    lastPrint = now;
    // Serial.println("");
    // printWiFiStatus();
  }

  readBME280Data();
  server.update();
}

// void printWiFiStatus() {
//   // print the SSID of the network you're attached to:
//   Serial.print("SSID: ");
//   Serial.println(WiFi.SSID());
//
//   // print your WiFi shield's IP address:
//   IPAddress ip = WiFi.localIP();
//   Serial.print("IP Address: ");
//   Serial.println(ip);
//
//   Serial.print("MDNS Name: ");
//   Serial.println(mdnsName);
//
//   Serial.print("Mac: ");
//   Serial.print(mac[5], HEX);
//   Serial.print(":");
//   Serial.print(mac[4], HEX);
//   Serial.print(":");
//   Serial.print(mac[3], HEX);
//   Serial.print(":");
//   Serial.print(mac[2], HEX);
//   Serial.print(":");
//   Serial.print(mac[1], HEX);
//   Serial.print(":");
//   Serial.println(mac[0], HEX);
//
//   // print the received signal strength:
//   long rssi = WiFi.RSSI();
//   Serial.print("signal strength (RSSI):");
//   Serial.print(rssi);
//   Serial.println(" dBm");
// }
