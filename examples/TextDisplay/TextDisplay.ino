/*********************************************************************
Web Thing which draws text provided as a property.
Adapted from the Adafruit SSD1306 example:

This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using SPI to communicate
4 or 5 pins are required to inteface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

const char* ssid = "...";
const char* password = "...";

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   2
#define OLED_CLK   16
#define OLED_DC    0
#define OLED_CS    13
#define OLED_RESET 15
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

/* Uncomment this block to use hardware SPI
#define OLED_DC     6
#define OLED_CS     7
#define OLED_RESET  8
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
*/

const int textHeight = 8;
const int textWidth = 6;
const int width = 128;
const int height = 64;

WebThingAdapter adapter("textdisplayer");

const char* textDisplayTypes[] = {"TextDisplay", nullptr};
ThingDevice textDisplay("textDisplay", "Text display", textDisplayTypes);
ThingProperty text("text", "", STRING, nullptr);

String lastText = "moz://a iot";

void displayString(const String& str) {
  int len = str.length();
  int strWidth = len * textWidth;
  int strHeight = textHeight;
  int scale = width / strWidth;
  if (strHeight > strWidth / 2) {
    scale = height / strHeight;
  }
  int x = width / 2 - strWidth * scale / 2;
  int y = height / 2 - strHeight * scale / 2;

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(scale);
  display.setCursor(x, y);
  display.println(str);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);

  // display the splashscreen as requested :)
  display.display();
  delay(2000);

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

  displayString(lastText);

  ThingPropertyValue value;
  value.string = &lastText;
  text.setValue(value);

  textDisplay.addProperty(&text);
  adapter.addDevice(&textDisplay);
  adapter.begin();
}

void loop() {
  adapter.update();
  displayString(lastText);
}

