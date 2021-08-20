#include <Arduino.h>
#include <Thing.h>
#include <WebThingAdapter.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
#define LARGE_JSON_BUFFERS 1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

const char *oledTypes[] = {"OLED Display", nullptr};
ThingDevice oledThing("oled", "OLED Display", oledTypes);

StaticJsonDocument<256> oledInput;
JsonObject oledInputObj = oledInput.to<JsonObject>();
ThingAction displayAction("display", "Display text", "display a text on OLED",
                 "displayAction", &oledInputObj, action_generator);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Initialize...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  Serial.println("Clear display...");

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

  adapter = new WebThingAdapter("oled-display", WiFi.localIP());
  oledThing.description = "A web connected oled display";

  
  oledInputObj["type"] = "object";
    JsonObject oledInputProperties =
      oledInputObj.createNestedObject("properties");


  JsonObject headlineInput =
      oledInputProperties.createNestedObject("headline");
  headlineInput["type"] = "string";
  
  JsonObject subheadlineInput =
      oledInputProperties.createNestedObject("subheadline");
  subheadlineInput["type"] = "string";

    JsonObject bodyInput =
      oledInputProperties.createNestedObject("body");
  bodyInput["type"] = "string";
  

  oledThing.addAction(&displayAction);

  adapter->addDevice(&oledThing);
  adapter->begin();

  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(oledThing.id);

}
 
#define HEADLINE_MAX_LENGHT 10
#define SUBHEAD_MAX_LENGTH 23
#define BODY_MAX_LENGTH 20

char headline[HEADLINE_MAX_LENGHT];
char subheadline[SUBHEAD_MAX_LENGTH];
char body[BODY_MAX_LENGTH];

void clear_buffer(){
  memset(headline, 0, HEADLINE_MAX_LENGHT);
  memset(subheadline, 0, SUBHEAD_MAX_LENGTH);
  memset(body, 0, BODY_MAX_LENGTH);
}

int rounds = 0;
 
void loop() 
{
  if(strlen(headline) > 0 || strlen(subheadline) > 0 || strlen(body) > 0){
    display.clearDisplay();
    delay(250);
    display.display();
    display.setCursor(0,0); 
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println(headline);

    display.setCursor(0,15);  
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println(subheadline);
    display.setCursor(0,30);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println(body);
    delay(250);
    display.display();
    clear_buffer();
    rounds = 1;
  }else if(rounds > 0){
      //To avoid burning, clear after 30 seconds.
      if(rounds == 300){
          display.clearDisplay();
          delay(250);
          display.display();
          rounds = 0;
      }else{
          delay(100);
          rounds++;
      }
  }
}

void do_display(const JsonVariant &input) {
  Serial.println("do display...");
   
  JsonObject inputObj = input.as<JsonObject>();
  display.clearDisplay();
  display.display();
  String headline_tmp = inputObj["headline"];
  String subheadline_tmp = inputObj["subheadline"];
  String body_tmp = inputObj["body"];
  clear_buffer();

  size_t length = strlen(headline_tmp.c_str());
  if(length > HEADLINE_MAX_LENGHT){
      Serial.println("Headline is too long");
      return;
  }else{
    memcpy(headline, headline_tmp.c_str(), length);
  }
  length = strlen(subheadline_tmp.c_str());
  if(length > SUBHEAD_MAX_LENGTH){
      Serial.println("Subheadline is too long");
      return;
  }else{
      memcpy(subheadline, subheadline_tmp.c_str(), length);
  }
  length = strlen(body_tmp.c_str());
  if(length > BODY_MAX_LENGTH){
      Serial.println("Body is too long");
      return;
  }else{
      memcpy(body, body_tmp.c_str(), length);
  }
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("display", input, do_display, nullptr);
}
