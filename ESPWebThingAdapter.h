/**
 * ESPWebThingAdapter.h
 *
 * Exposes the Web Thing API based on provided ThingDevices.
 * Suitable for ESP32 and ESP8266 using ESPAsyncWebServer and ESPAsyncTCP
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MOZILLA_IOT_ESPWEBTHINGADAPTER_H
#define MOZILLA_IOT_ESPWEBTHINGADAPTER_H

#if defined(ESP32) || defined(ESP8266)

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif
#include "Thing.h"

#define ESP_MAX_PUT_BODY_SIZE 512

class WebThingAdapter {
public:
  WebThingAdapter(String _name, IPAddress _ip, uint16_t _port = 80): server(_port), name(_name), ip(_ip.toString()), port(_port) {
  }

  void begin() {
    name.toLowerCase();
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("webthing", "tcp", port);
    MDNS.addServiceTxt("webthing", "tcp", "path", "/");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS");

    this->server.onNotFound(std::bind(&WebThingAdapter::handleUnknown, this, std::placeholders::_1));

    this->server.on("/", HTTP_GET, std::bind(&WebThingAdapter::handleThings, this, std::placeholders::_1));

    ThingDevice* device = this->firstDevice;
    while (device != nullptr) {
      String deviceBase = "/things/" + device->id;

      ThingProperty* property = device->firstProperty;
      while (property != nullptr) {
        String propertyBase = deviceBase + "/properties/" + property->id;
        this->server.on(propertyBase.c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThingGetItem, this, std::placeholders::_1, property));
        this->server.on(propertyBase.c_str(), HTTP_PUT,
          std::bind(&WebThingAdapter::handleThingPropertyPut, this, std::placeholders::_1, property),
          NULL,
          std::bind(&WebThingAdapter::handleBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
        );

        property = (ThingProperty*) property->next;
      }

      ThingEvent* event = device->firstEvent;
      while (event != nullptr) {
        String eventBase = deviceBase + "/events/" + event->id;
        this->server.on(eventBase.c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThingGetItem, this, std::placeholders::_1, event));
        event = (ThingEvent*) event->next;
      }

      this->server.on((deviceBase + "/properties").c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThingGetAll, this, std::placeholders::_1, device, device->firstProperty));
      this->server.on((deviceBase + "/events").c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThingGetAll, this, std::placeholders::_1, device, device->firstEvent));
      this->server.on(deviceBase.c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThing, this, std::placeholders::_1, device));

      device = device->next;

    }

    this->server.begin();
  }

#ifndef WITHOUT_WS
  void sendChangedPropsOrEvents(ThingDevice* device, const char* type, ThingItem* rootItem) {
    // Prepare one buffer per device
    DynamicJsonBuffer buf(1024);
    JsonObject& message = buf.createObject();
    message["messageType"] = type;
    JsonObject& prop = message.createNestedObject("data");
    bool dataToSend = false;
    ThingItem* item = rootItem;
    while (item != nullptr) {
      ThingPropertyValue* value = item->changedValueOrNull();
      if (value) {
        dataToSend = true;
        this->serializeThingItem(item, prop);
      }
      item = item->next;
    }
    if (dataToSend) {
      String jsonStr;
      message.printTo(jsonStr);
      // Inform all connected ws clients of a Thing about changed properties
      ((AsyncWebSocket*)device->ws)->textAll(jsonStr.c_str(), jsonStr.length());
    }
  }
#endif

  void update() {
#ifdef ESP8266
    MDNS.update();
#endif
#ifndef WITHOUT_WS
  // * Send changed properties as defined in "4.5 propertyStatus message"
  // * Send events as defined in "4.7 event message"
  // Do this by looping over all devices and properties/events
  ThingDevice* device = this->firstDevice;
  while (device != nullptr) {
    sendChangedPropsOrEvents(device, "propertyStatus", device->firstProperty);
    sendChangedPropsOrEvents(device, "event", device->firstEvent);
    device = device->next;
  }
#endif
  }

  void addDevice(ThingDevice* device) {
    if (this->lastDevice == nullptr) {
      this->firstDevice = device;
      this->lastDevice = device;
    } else {
      this->lastDevice->next = device;
      this->lastDevice = device;
    }
    // Initiate the websocket instance
    #ifndef WITHOUT_WS
    AsyncWebSocket* ws = new AsyncWebSocket("/things/" + device->id);
    device->ws = ws;
    // AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len, ThingDevice* device
    ws->onEvent(std::bind(&WebThingAdapter::handleWS, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, device));
    this->server.addHandler(ws);
    #endif
  }

private:
  AsyncWebServer server;

  String name;
  String ip;
  uint16_t port;
  ThingDevice* firstDevice = nullptr;
  ThingDevice* lastDevice = nullptr;
  char body_data[ESP_MAX_PUT_BODY_SIZE];
  bool b_has_body_data = false;

  bool verifyHost(AsyncWebServerRequest *request) {
    AsyncWebHeader* header = request->getHeader("Host");
    if (header == nullptr) {
      request->send(403);
      return false;
    }
    String value = header->value();
    int colonIndex = value.indexOf(':');
    if (colonIndex >= 0) {
      value.remove(colonIndex);
    }
    if (value == name + ".local" || value == ip) {
      return true;
    }
    request->send(403);
    return false;
  }

  #ifndef WITHOUT_WS
  void sendErrorMsg(DynamicJsonBuffer &buffer, AsyncWebSocketClient& client, int status, const char* msg) {
      JsonObject& prop = buffer.createObject();
      prop["error"] = msg;
      prop["status"] = status;
      String jsonStr;
      prop.printTo(jsonStr);
      client.text(jsonStr.c_str(), jsonStr.length());
  }

  void handleWS(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *rawData, size_t len, ThingDevice* device) {
    // Ignore all except data packets
    if(type != WS_EVT_DATA) return;

    // Only consider non fragmented data
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(!info->final || info->index != 0 || info->len != len) return;

    // Web Thing only specifies text, not binary websocket transfers
    if(info->opcode != WS_TEXT) return;

    // In theory we could just have one websocket for all Things and react on the server->url() to route data.
    // Controllers will however establish a separate websocket connection for each Thing anyway as of in the
    // spec. For now each Thing stores its own Websocket connection object therefore.

    // Parse request
    DynamicJsonBuffer newBuffer(1024);
    JsonObject& newProp = newBuffer.parseObject(rawData);
    if (!newProp.success()) {
      sendErrorMsg(newBuffer, *client, 400, "Invalid json");
      return;
    }

    String messageType = newProp["messageType"].as<String>();
    const JsonVariant& dataVariant = newProp["data"];
    if (!dataVariant.is<JsonObject>()) {
      sendErrorMsg(newBuffer, *client, 400, "data must be an object");
      return;
    }

    const JsonObject &data = dataVariant.as<const JsonObject&>();

    if (messageType == "setProperty") {
      for (auto kv : data) {
        ThingProperty* property = device->findProperty(kv.key);
        if (property) {
          setThingProperty(data, property);
        }
      }

      // Send confirmation by sending back the received property object
      String jsonStr;
      data.printTo(jsonStr);
      client->text(jsonStr.c_str(), jsonStr.length());
    } else if (messageType == "requestAction") {
      sendErrorMsg(newBuffer, *client, 400, "Not supported yet");
    } else if (messageType == "addEventSubscription") {
      // We report back all property state changes. We'd require a map
      // of subscribed properties per websocket connection otherwise
    }
  }
  #endif

  void handleUnknown(AsyncWebServerRequest *request) {
    if (!verifyHost(request)) {
      return;
    }
    request->send(404);
  }

  void handleThings(AsyncWebServerRequest *request) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonBuffer buf(1024);
    JsonArray& things = buf.createArray();
    ThingDevice* device = this->firstDevice;
    while (device != nullptr) {
      JsonObject& descr = things.createNestedObject();
      this->serializeDevice(descr, device);
      descr["href"] = "/things/" + device->id;
      device = device->next;
    }

    things.printTo(*response);
    request->send(response);

  }

  void serializePropertyOrEvent(JsonObject& descr, ThingDevice* device, const char* type, bool isProp, ThingItem* item) {
    String basePath = "/things/" + device->id + "/"+ type + "/";
    JsonObject& props = descr.createNestedObject(type);
    while (item != nullptr) {
      JsonObject& prop = props.createNestedObject(item->id);
      switch (item->type) {
      case NO_STATE:
        break;
      case BOOLEAN:
        prop["type"] = "boolean";
        break;
      case NUMBER:
        prop["type"] = "number";
        break;
      case STRING:
        prop["type"] = "string";
        break;
      }

      if (item->readOnly) {
        prop["readOnly"] = true;
      }

      if (item->unit != "") {
        prop["unit"] = item->unit;
      }

      if (item->title != "") {
        prop["title"] = item->title;
      }

      if (item->minimum < item->maximum) {
        prop["minimum"] = item->minimum;
      }

      if (item->maximum > item->minimum) {
        prop["maximum"] = item->maximum;
      }

      if (item->multipleOf > 0) {
        prop["multipleOf"] = item->multipleOf;
      }

      if (isProp) {
        ThingProperty* property = (ThingProperty*)item;
        const char **enumVal = property->propertyEnum;
        bool hasEnum = (property->propertyEnum != nullptr) && ((*property->propertyEnum) != nullptr);

        if (hasEnum) {
          enumVal = property->propertyEnum;
          JsonArray &propEnum = prop.createNestedArray("enum");
          while (property->propertyEnum != nullptr && (*enumVal) != nullptr){
            propEnum.add(*enumVal);
            enumVal++;
          }
        }
      }

      if (item->atType != nullptr) {
        prop["@type"] = item->atType;
      }

      // 2.9 Property object: A links array (An array of Link objects linking to one or more representations of a Property resource, each with an implied default rel=property.)
      JsonArray& inline_links = prop.createNestedArray("links");
      JsonObject& inline_links_prop = inline_links.createNestedObject();
      inline_links_prop["href"] = basePath + item->id;

      item = item->next;
    }
  }

  void serializeDevice(JsonObject& descr, ThingDevice* device) {
    descr["id"] = device->id;
    descr["title"] = device->title;
    descr["@context"] = "https://iot.mozilla.org/schemas";
    // TODO: descr["base"] = ???

    JsonObject& securityDefinitions = descr.createNestedObject("securityDefinitions");
    JsonObject& nosecSc = securityDefinitions.createNestedObject("nosec_sc");
    nosecSc["scheme"] = "nosec";

    JsonArray& typeJson = descr.createNestedArray("@type");
    const char** type = device->type;
    while ((*type) != nullptr) {
      typeJson.add(*type);
      type++;
    }

    JsonArray& links = descr.createNestedArray("links");
    {
      JsonObject& links_prop = links.createNestedObject();
      links_prop["rel"] = "properties";
      links_prop["href"] = "/things/" + device->id + "/properties";
    }

    {
      JsonObject& links_prop = links.createNestedObject();
      links_prop["rel"] = "events";
      links_prop["href"] = "/things/" + device->id + "/events";
    }

#ifndef WITHOUT_WS
    {
      JsonObject& links_prop = links.createNestedObject();
      links_prop["rel"] = "alternate";
      char buffer [33];
      itoa(port, buffer, 10);
      links_prop["href"] = "ws://" + ip + ":" + buffer + "/things/" + device->id;
    }
#endif

    ThingProperty* property = device->firstProperty;
    if (property) {
      serializePropertyOrEvent(descr, device, "properties", true, property);
    }

    ThingEvent* event = device->firstEvent;
    if (event) {
      serializePropertyOrEvent(descr, device, "events", false, event);
    }
  }

  void handleThing(AsyncWebServerRequest *request, ThingDevice*& device) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonBuffer buf(1024);
    JsonObject& descr = buf.createObject();
    this->serializeDevice(descr, device);

    descr.printTo(*response);
    request->send(response);
  }

  void serializeThingItem(ThingItem* item, JsonObject& prop) {
    switch (item->type) {
    case NO_STATE:
      break;
    case BOOLEAN:
      prop[item->id] = item->getValue().boolean;
      break;
    case NUMBER:
      prop[item->id] = item->getValue().number;
      break;
    case STRING:
      prop[item->id] = *item->getValue().string;
      break;
    }
  }

  void handleThingGetItem(AsyncWebServerRequest *request, ThingItem* item) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonBuffer buf(256);
    JsonObject& prop = buf.createObject();
    serializeThingItem(item, prop);
    prop.printTo(*response);
    request->send(response);
  }

  void handleThingGetAll(AsyncWebServerRequest *request, ThingDevice* device, ThingItem* rootItem) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonBuffer buf(256);
    JsonObject& prop = buf.createObject();
    ThingItem *item = rootItem;
    while (item != nullptr) {
      serializeThingItem(item, prop);
      item = item->next;
    }
    prop.printTo(*response);
    request->send(response);
  }

  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (total >= ESP_MAX_PUT_BODY_SIZE || index + len >= ESP_MAX_PUT_BODY_SIZE) {
        return; // cannot store this size..
    }
    // copy to internal buffer
    memcpy(&body_data[index], data, len);
    b_has_body_data = true;
  }

  void setThingProperty(const JsonObject& newProp, ThingProperty* property) {
    const JsonVariant newValue = newProp[property->id];

    switch (property->type) {
    case NO_STATE: {
      break;
    }
    case BOOLEAN: {
      ThingPropertyValue value;
      value.boolean = newValue.as<bool>();
      property->setValue(value);
      break;
    }
    case NUMBER: {
      ThingPropertyValue value;
      value.number = newValue.as<double>();
      property->setValue(value);
      break;
    }
    case STRING:
      *(property->getValue().string) = newValue.as<String>();
      break;
    }
  }

  void handleThingPropertyPut(AsyncWebServerRequest *request, ThingProperty* property) {
    if (!verifyHost(request)) {
      return;
    }
    if (!b_has_body_data) {
      request->send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonBuffer newBuffer(256);
    JsonObject& newProp = newBuffer.parseObject(body_data);
    if (!newProp.success()) { // unable to parse json
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      request->send(500);
      return;
    }

    setThingProperty(newProp, property);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    newProp.printTo(*response);
    request->send(response);

    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));
  }

};

#endif    // ESP

#endif
