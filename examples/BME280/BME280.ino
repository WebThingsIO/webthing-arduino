#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <BME280.h>
#include <BME280I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Thing.h>
#include <WebThingAdapter.h>

#ifndef PIN_STATE_HIGH
#define PIN_STATE_HIGH HIGH
#endif
#ifndef PIN_STATE_LOW
#define PIN_STATE_LOW LOW
#endif

#define ESP32

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "";
const char *password = "";

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13; // manually configure LED pin
#endif

WebThingAdapter *adapter;

const char *bme280Types[] = {"TemperatureSensor", nullptr};
ThingDevice weather("bme280", "BME280 Weather Sensor", bme280Types);
ThingProperty weatherTemp("temperature", "", NUMBER, "TemperatureProperty");
// Set humidity as level-property
ThingProperty weatherHum("humidity", "", NUMBER, "LevelProperty");
ThingProperty weatherPres("pressure", "", NUMBER, nullptr);

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
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
  Serial.println("Initialize...");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
  WiFi.begin(ssid, password);
  
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

  Wire.begin();
  while (!bme.begin()) {
    Serial.println("Could not find BME280I2C sensor!");
    delay(1000);
  }
  Serial.println("BME280I2C connected and initialized.");

  // connected, make the LED stay on
  digitalWrite(LED_BUILTIN, PIN_STATE_HIGH);
  adapter = new WebThingAdapter("weathersensor", WiFi.localIP());

  // Set unit for temperature
  weatherTemp.unit = "degree celsius";

  // Set title to "Pressure"
  weatherPres.title = "Pressure";
  // Set unit for pressure to hPa
  weatherPres.unit = "hPa";
  // Set pressure to read only
  weatherPres.readOnly = "true";

  // Set title to "Humidity"
  weatherHum.title = "Humidity";
  // Set unit for humidity to %
  weatherHum.unit = "percent";
  // Set humidity as read only
  weatherHum.readOnly = "true";
  // Set min and max for LevelProperty
  weatherHum.minimum = 0;
  weatherHum.maximum = 100;

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