#include <AtWiFi.h>
#include "Thing.h"
#include "WebThingAdapter.h"

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "public";
const char *password = "";

const char *deviceTypes[] = {"MultiLevelSensor", nullptr};
ThingDevice device("WioTerminal", "Wio Terminal", deviceTypes);
ThingProperty property("lightLevel", "Light level", NUMBER, "LevelProperty");
WebThingAdapter *adapter = NULL;

double lastValue = 0;

void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    ;

  setupNetwork();

  adapter = new WebThingAdapter("light-sensor", WiFi.localIP());
  property.readOnly = true;
  property.title = "Light Level";
  device.addProperty(&property);
  adapter->addDevice(&device);

  Serial.println("Starting HTTP server");
  adapter->begin();
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(device.id);
}

void loop(void) {
  int value = analogRead(WIO_LIGHT);

  if (lastValue != value) {
    ThingPropertyValue levelValue;
    levelValue.number = value;
    property.setValue(levelValue);
    lastValue = value;
  }

  adapter->update();
}
