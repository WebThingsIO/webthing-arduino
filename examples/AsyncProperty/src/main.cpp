/**
 * Test of webthing-arduino with async behavior. Works with
 * thing-url-adapter.
 */

#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>

const char *ssid = "XXXXXXX";
const char *password = "XXXXXXX";

int loginCounter;
WebThingAdapter *adapter;
// Forward declaration
void onOffChanged(ThingPropertyValue newValue);
void textChanged(ThingPropertyValue newValue);
void numberChanged(ThingPropertyValue newValue);

const char *asyncProperties[] = {"asyncProperty", nullptr};
ThingDevice textDisplay("asyncProperty", "AsyncProperty Test",
                        asyncProperties);
ThingProperty text("text", "", STRING, nullptr, textChanged);
ThingProperty onOff("bool", "", BOOLEAN, "OnOffProperty", onOffChanged);
ThingProperty number("number", "", NUMBER, nullptr, numberChanged);

String message = "message";
String lastMessage = message;
boolean lastState = false;
int lastNumber = 0;

// Callback functions
//
void onOffChanged(ThingPropertyValue newValue) {
  Serial.print("On/Off changed to : ");
  Serial.println(newValue.boolean);
}

void textChanged(ThingPropertyValue newValue) {
  Serial.print("New message : ");
  Serial.println(*newValue.string);
}

void numberChanged(ThingPropertyValue newValue) {
  Serial.print("New number : ");
  Serial.println(newValue.number);
}

void setup() {
  Serial.begin(115200);

  // counter used for login time out, function needed with ESP32
  loginCounter = 0;

#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif

  Serial.print("Connecting : ");
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {

    // more than 5 sec wait, try to login again.
    if (loginCounter == 10) {
      loginCounter = 0;
      WiFi.begin(ssid, password);
    } else {
      loginCounter++;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ThingPropertyValue value;
  value.string = &message;
  text.setValue(value);

  textDisplay.addProperty(&text);
  textDisplay.addProperty(&onOff);
  textDisplay.addProperty(&number);
  
  adapter = new WebThingAdapter(&textDisplay, "asyncProperty", WiFi.localIP());
  adapter->begin();
}

void loop() {
  delay(500);
  adapter->update();
}
