/**
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

#define LARGE_JSON_BUFFERS 1

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "";
const char *password = "";

#if defined(LED_BUILTIN)
const int ledPin = LED_BUILTIN;
#else
const int ledPin = 13; // manually configure LED pin
#endif

ThingActionObject *action_generator(DynamicJsonDocument *);
WebThingAdapter *adapter;

const char *displayTypes[] = {"Matrix Display", nullptr};
ThingDevice displayThing("matrixDisplay", "Matrix Display", displayTypes);

StaticJsonDocument<256> displayInput;
JsonObject displayInputObj = displayInput.to<JsonObject>();
ThingAction displayAction("display", "Display text", "display a text on a Matrix display",
                 "displayAction", &displayInputObj, action_generator);

void setup() {
  Serial.begin(115200);
  Serial.println("Initialize...");
  myDisplay.begin();
  myDisplay.setIntensity(0);
  Serial.println("Clear display...");
  myDisplay.displayClear();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(ssid, password);
  
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH);
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  adapter = new WebThingAdapter("matrix-display", WiFi.localIP());
  displayThing.description = "A web connected matrix display";

  
  displayInputObj["type"] = "object";
    JsonObject displayInputProperties =
      displayInputObj.createNestedObject("properties");


  JsonObject bodyInput =
      displayInputProperties.createNestedObject("body");
  bodyInput["type"] = "string";

  displayThing.addAction(&displayAction);

  adapter->addDevice(&displayThing);
  adapter->begin();
}

#define BODY_MAX_LENGTH 200
#define MAX_WO_SCROLLING 6

char body[BODY_MAX_LENGTH];
bool scrolling = false;

void clear_buffer(){
  memset(body, 0, BODY_MAX_LENGTH);
  scrolling = false;
}

int rounds = 0;
 
void loop() 
{
  if(strlen(body) > 0){
    myDisplay.displayClear();
    myDisplay.setTextAlignment(PA_LEFT);

    myDisplay.print(body);
    rounds = 1;
    clear_buffer();
  }else if(rounds > 0){
      //To avoid burning, clear after 30 seconds.
      if(rounds == 300){
          myDisplay.displayClear();
          rounds = 0;
      }else{
          delay(100);
          rounds++;
      }
  }
}

void do_display(const JsonVariant &input) {
  Serial.println("do display...");
  clear_buffer();
   
  JsonObject inputObj = input.as<JsonObject>();
  myDisplay.displayClear();
  String body_tmp = inputObj["body"];

  size_t length = strlen(body_tmp.c_str());
  if(length > BODY_MAX_LENGTH){
      Serial.println("Body is too long");
      return;
  } else{
      memcpy(body, body_tmp.c_str(), length);
  }
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("display", input, do_display, nullptr);
}