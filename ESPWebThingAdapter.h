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

#pragma once

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
  WebThingAdapter(String _name, IPAddress _ip, uint16_t _port = 80)
      : server(_port), name(_name), ip(_ip.toString()), port(_port) {}

  void begin() {
    name.toLowerCase();
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("webthing", "tcp", port);
    MDNS.addServiceTxt("webthing", "tcp", "path", "/");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                         "PUT, GET, OPTIONS");

    this->server.onNotFound(std::bind(&WebThingAdapter::handleUnknown, this,
                                      std::placeholders::_1));

    this->server.on("/", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThings, this,
                              std::placeholders::_1));

    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      String deviceBase = "/things/" + device->id;

      ThingProperty *property = device->firstProperty;
      while (property != nullptr) {
        String propertyBase = deviceBase + "/properties/" + property->id;
        this->server.on(propertyBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingGetItem, this,
                                  std::placeholders::_1, property));
        this->server.on(propertyBase.c_str(), HTTP_PUT,
                        std::bind(&WebThingAdapter::handleThingPropertyPut,
                                  this, std::placeholders::_1, property),
                        NULL,
                        std::bind(&WebThingAdapter::handleBody, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3, std::placeholders::_4,
                                  std::placeholders::_5));

        property = (ThingProperty *)property->next;
      }

      ThingEvent *event = device->firstEvent;
      while (event != nullptr) {
        String eventBase = deviceBase + "/events/" + event->id;
        this->server.on(eventBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingGetItem, this,
                                  std::placeholders::_1, event));
        event = (ThingEvent *)event->next;
      }

      this->server.on((deviceBase + "/properties").c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingGetAll, this,
                                std::placeholders::_1, device->firstProperty));
      this->server.on((deviceBase + "/events").c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingGetAll, this,
                                std::placeholders::_1, device->firstEvent));
      this->server.on(deviceBase.c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThing, this,
                                std::placeholders::_1, device));

      device = device->next;
    }

    this->server.begin();
  }

#ifndef WITHOUT_WS
  void sendChangedPropsOrEvents(ThingDevice *device, const char *type,
                                ThingItem *rootItem) {
    // Prepare one buffer per device
    DynamicJsonDocument message(1024);
    message["messageType"] = type;
    JsonObject prop = message.createNestedObject("data");
    bool dataToSend = false;
    ThingItem *item = rootItem;
    while (item != nullptr) {
      ThingPropertyValue *value = item->changedValueOrNull();
      if (value) {
        dataToSend = true;
        item->serialize(prop);
      }
      item = item->next;
    }
    if (dataToSend) {
      String jsonStr;
      serializeJson(message, jsonStr);
      // Inform all connected ws clients of a Thing about changed properties
      ((AsyncWebSocket *)device->ws)->textAll(jsonStr);
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
    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      sendChangedPropsOrEvents(device, "propertyStatus",
                               device->firstProperty);
      sendChangedPropsOrEvents(device, "event", device->firstEvent);
      device = device->next;
    }
#endif
  }

  void addDevice(ThingDevice *device) {
    if (this->lastDevice == nullptr) {
      this->firstDevice = device;
      this->lastDevice = device;
    } else {
      this->lastDevice->next = device;
      this->lastDevice = device;
    }
// Initiate the websocket instance
#ifndef WITHOUT_WS
    AsyncWebSocket *ws = new AsyncWebSocket("/things/" + device->id);
    device->ws = ws;
    // AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType
    // type, void * arg, uint8_t *data, size_t len, ThingDevice* device
    ws->onEvent(std::bind(
        &WebThingAdapter::handleWS, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
        std::placeholders::_5, std::placeholders::_6, device));
    this->server.addHandler(ws);
#endif
  }

private:
  AsyncWebServer server;

  String name;
  String ip;
  uint16_t port;
  ThingDevice *firstDevice = nullptr;
  ThingDevice *lastDevice = nullptr;
  char body_data[ESP_MAX_PUT_BODY_SIZE];
  bool b_has_body_data = false;

  bool verifyHost(AsyncWebServerRequest *request) {
    AsyncWebHeader *header = request->getHeader("Host");
    if (header == nullptr) {
      request->send(403);
      return false;
    }
    String value = header->value();
    int colonIndex = value.indexOf(':');
    if (colonIndex >= 0) {
      value.remove(colonIndex);
    }
    if (value == name + ".local" || value == ip || value == "localhost") {
      return true;
    }
    request->send(403);
    return false;
  }

#ifndef WITHOUT_WS
  void sendErrorMsg(DynamicJsonDocument &prop, AsyncWebSocketClient &client,
                    int status, const char *msg) {
    prop["error"] = msg;
    prop["status"] = status;
    String jsonStr;
    serializeJson(prop, jsonStr);
    client.text(jsonStr.c_str(), jsonStr.length());
  }

  void handleWS(AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *rawData, size_t len,
                ThingDevice *device) {
    // Ignore all except data packets
    if (type != WS_EVT_DATA)
      return;

    // Only consider non fragmented data
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!info->final || info->index != 0 || info->len != len)
      return;

    // Web Thing only specifies text, not binary websocket transfers
    if (info->opcode != WS_TEXT)
      return;

    // In theory we could just have one websocket for all Things and react on
    // the server->url() to route data. Controllers will however establish a
    // separate websocket connection for each Thing anyway as of in the spec.
    // For now each Thing stores its own Websocket connection object therefore.

    // Parse request
    DynamicJsonDocument newProp(1024);
    auto error = deserializeJson(newProp, rawData);
    if (error) {
      sendErrorMsg(newProp, *client, 400, "Invalid json");
      return;
    }

    String messageType = newProp["messageType"].as<String>();
    JsonVariant dataVariant = newProp["data"];
    if (!dataVariant.is<JsonObject>()) {
      sendErrorMsg(newProp, *client, 400, "data must be an object");
      return;
    }

    JsonObject data = dataVariant.as<JsonObject>();

    if (messageType == "setProperty") {
      for (auto kv : data) {
        ThingProperty *property = device->findProperty(kv.key().c_str());
        if (property) {
          setThingProperty(data, property);
        }
      }
    } else if (messageType == "requestAction") {
      sendErrorMsg(newProp, *client, 400, "Not supported yet");
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
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument buf(1024);
    JsonArray things = buf.to<JsonArray>();
    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      JsonObject descr = things.createNestedObject();
      device->serialize(descr
#ifndef WITHOUT_WS
                        ,
                        ip, port
#endif
      );
      descr["href"] = "/things/" + device->id;
      device = device->next;
    }

    serializeJson(things, *response);
    request->send(response);
  }

  void handleThing(AsyncWebServerRequest *request, ThingDevice *&device) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument buf(1024);
    JsonObject descr = buf.to<JsonObject>();
    device->serialize(descr
#ifndef WITHOUT_WS
                      ,
                      ip, port
#endif
    );

    serializeJson(descr, *response);
    request->send(response);
  }

  void handleThingGetItem(AsyncWebServerRequest *request, ThingItem *item) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(256);
    JsonObject prop = doc.to<JsonObject>();
    item->serialize(prop);
    serializeJson(prop, *response);
    request->send(response);
  }

  void handleThingGetAll(AsyncWebServerRequest *request, ThingItem *rootItem) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(256);
    JsonObject prop = doc.to<JsonObject>();
    ThingItem *item = rootItem;
    while (item != nullptr) {
      item->serialize(prop);
      item = item->next;
    }
    serializeJson(prop, *response);
    request->send(response);
  }

  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
                  size_t index, size_t total) {
    if (total >= ESP_MAX_PUT_BODY_SIZE ||
        index + len >= ESP_MAX_PUT_BODY_SIZE) {
      return; // cannot store this size..
    }
    // copy to internal buffer
    memcpy(&body_data[index], data, len);
    b_has_body_data = true;
  }

  void setThingProperty(const JsonObject newProp, ThingProperty *property) {
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
    case INTEGER: {
      ThingPropertyValue value;
      value.integer = newValue.as<signed long long>();
      property->setValue(value);
      break;
    }
    case STRING:
      *(property->getValue().string) = newValue.as<String>();
      break;
    }
  }

  void handleThingPropertyPut(AsyncWebServerRequest *request,
                              ThingProperty *property) {
    if (!verifyHost(request)) {
      return;
    }
    if (!b_has_body_data) {
      request->send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument newBuffer(256);
    auto error = deserializeJson(newBuffer, body_data);
    if (error) { // unable to parse json
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      request->send(500);
      return;
    }
    JsonObject newProp = newBuffer.as<JsonObject>();

    setThingProperty(newProp, property);

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    serializeJson(newProp, *response);
    request->send(response);

    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));
  }
};

#endif // ESP
