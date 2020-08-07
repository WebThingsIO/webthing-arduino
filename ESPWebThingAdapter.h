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

#ifndef LARGE_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define LARGE_JSON_DOCUMENT_SIZE 4096
#else
#define LARGE_JSON_DOCUMENT_SIZE 1024
#endif
#endif

#ifndef SMALL_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define SMALL_JSON_DOCUMENT_SIZE 1024
#else
#define SMALL_JSON_DOCUMENT_SIZE 256
#endif
#endif

class WebThingAdapter {
public:
  WebThingAdapter(ThingDevice *_thing, String _name, IPAddress _ip,
                  uint16_t _port = 80)
      : thing(_thing), server(_port), name(_name), ip(_ip.toString()),
        port(_port) {}

  void begin() {
    // TODO: Code from addDevice to add thing to WS etc
    name.toLowerCase();
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("labthing", "tcp", port);
    MDNS.addServiceTxt("labthing", "tcp", "path", "/");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                         "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader(
        "Access-Control-Allow-Headers",
        "Origin, X-Requested-With, Content-Type, Accept");

    this->server.onNotFound(std::bind(&WebThingAdapter::handleUnknown, this,
                                      std::placeholders::_1));

    this->server.on("/*", HTTP_OPTIONS,
                    std::bind(&WebThingAdapter::handleOptions, this,
                              std::placeholders::_1));

    ThingDevice *device = this->thing;

    this->server.on("/", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThing, this,
                              std::placeholders::_1, device));

    ThingProperty *property = this->thing->firstProperty;
    while (property != nullptr) {
      String propertyBase = "/properties/" + property->id;
      this->server.on(propertyBase.c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingPropertyGet, this,
                                std::placeholders::_1, property));
      this->server.on(propertyBase.c_str(), HTTP_PUT,
                      std::bind(&WebThingAdapter::handleThingPropertyPut, this,
                                std::placeholders::_1, this->thing, property),
                      NULL,
                      std::bind(&WebThingAdapter::handleBody, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3, std::placeholders::_4,
                                std::placeholders::_5));

      property = (ThingProperty *)property->next;
    }

    ThingAction *action = this->thing->firstAction;
    while (action != nullptr) {
      String actionBase = "/actions/" + action->id;
      this->server.on(actionBase.c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingActionGet, this,
                                std::placeholders::_1, this->thing, action));
      this->server.on(actionBase.c_str(), HTTP_POST,
                      std::bind(&WebThingAdapter::handleThingActionPost, this,
                                std::placeholders::_1, this->thing, action),
                      NULL,
                      std::bind(&WebThingAdapter::handleBody, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3, std::placeholders::_4,
                                std::placeholders::_5));
      this->server.on(actionBase.c_str(), HTTP_DELETE,
                      std::bind(&WebThingAdapter::handleThingActionDelete,
                                this, std::placeholders::_1, this->thing,
                                action));
      action = (ThingAction *)action->next;
    }

    ThingEvent *event = this->thing->firstEvent;
    while (event != nullptr) {
      String eventBase = "/events/" + event->id;
      this->server.on(eventBase.c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingEventGet, this,
                                std::placeholders::_1, this->thing, event));
      event = (ThingEvent *)event->next;
    }

    this->server.on("/properties", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThingPropertiesGet, this,
                              std::placeholders::_1,
                              this->thing->firstProperty));
    this->server.on("/actions", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThingActionsGet, this,
                              std::placeholders::_1, this->thing));
    this->server.on("/events", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThingEventsGet, this,
                              std::placeholders::_1, this->thing));

#ifndef WITHOUT_WS
    // Initiate the websocket instance
    AsyncWebSocket *ws = new AsyncWebSocket("/ws");
    device->ws = ws;
    // AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType
    // type, void * arg, uint8_t *data, size_t len, ThingDevice* device
    ws->onEvent(std::bind(
        &WebThingAdapter::handleWS, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
        std::placeholders::_5, std::placeholders::_6, device));
    this->server.addHandler(ws);
#endif

    this->server.begin();
  }

  void update() {
#ifdef ESP8266
    MDNS.update();
#endif
#ifndef WITHOUT_WS
    // * Send changed properties as defined in "4.5 propertyStatus message"
    // Do this by looping over all devices and properties
    sendChangedProperties(this->thing);
#endif
  }

private:
  ThingDevice *thing = nullptr;
  AsyncWebServer server;
  String name;
  String ip;
  uint16_t port;

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
                AwsEventType type, void *arg, const uint8_t *rawData,
                size_t len, ThingDevice *device) {
    if (type == WS_EVT_DISCONNECT || type == WS_EVT_ERROR) {
      device->removeEventSubscriptions(client->id());
      return;
    }

    // Ignore all others except data packets
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
    DynamicJsonDocument newProp(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(newProp, rawData, len);
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
      for (JsonPair kv : data) {
        device->setProperty(kv.key().c_str(), kv.value());
      }
    } else if (messageType == "requestAction") {
      for (JsonPair kv : data) {
        DynamicJsonDocument *actionRequest =
            new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);

        JsonObject actionObj = actionRequest->to<JsonObject>();

        for (JsonPair kvInner : kv.value().as<JsonObject>()) {
          actionObj[kvInner.key()] = kvInner.value();
        }

        ThingActionObject *obj =
            device->requestAction(kv.key().c_str(), actionRequest);
        if (obj != nullptr) {
          obj->setNotifyFunction(std::bind(&ThingDevice::sendActionStatus,
                                           device, std::placeholders::_1));
          device->sendActionStatus(obj);

          obj->start();
        }
      }
    } else if (messageType == "addEventSubscription") {
      for (JsonPair kv : data) {
        ThingEvent *event = device->findEvent(kv.key().c_str());
        if (event) {
          device->addEventSubscription(client->id(), event->id);
        }
      }
    }
  }

  void sendChangedProperties(ThingDevice *device) {
    // Prepare one buffer per device
    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
    message["messageType"] = "propertyStatus";
    JsonObject prop = message.createNestedObject("data");
    bool dataToSend = false;
    ThingItem *item = device->firstProperty;
    while (item != nullptr) {
      ThingDataValue *value = item->changedValueOrNull();
      if (value) {
        dataToSend = true;
        item->serializeValueToObject(prop);
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

  void handleUnknown(AsyncWebServerRequest *request) {
    if (!verifyHost(request)) {
      return;
    }
    request->send(404);
  }

  void handleOptions(AsyncWebServerRequest *request) {
    if (!verifyHost(request)) {
      return;
    }
    request->send(204);
  }

  void handleThing(AsyncWebServerRequest *request, ThingDevice *&device) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject descr = buf.to<JsonObject>();
    device->serialize(descr, ip, port);

    serializeJson(descr, *response);
    request->send(response);
  }

  void handleThingPropertyGet(AsyncWebServerRequest *request,
                              ThingItem *item) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonVariant variant = doc.to<JsonVariant>();
    item->serializeValueToVariant(variant);
    serializeJson(doc, *response);
    request->send(response);
  }

  void handleThingActionGet(AsyncWebServerRequest *request,
                            ThingDevice *device, ThingAction *action) {
    if (!verifyHost(request)) {
      return;
    }

    String url = request->url();
    String base = "/actions/" + action->id;
    if (url == base || url == base + "/") {
      AsyncResponseStream *response =
          request->beginResponseStream("application/json");
      DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
      JsonArray queue = doc.to<JsonArray>();
      device->serializeActionQueue(queue, action->id);
      serializeJson(queue, *response);
      request->send(response);
    } else {
      String actionId = url.substring(base.length() + 1);
      const char *actionIdC = actionId.c_str();
      const char *slash = strchr(actionIdC, '/');

      if (slash) {
        actionId = actionId.substring(0, slash - actionIdC);
      }

      // TODO: Remove action key from response

      ThingActionObject *obj = device->findActionObject(actionId.c_str());
      if (obj == nullptr) {
        request->send(404);
        return;
      }

      AsyncResponseStream *response =
          request->beginResponseStream("application/json");
      DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
      JsonObject o = doc.to<JsonObject>();
      obj->serialize(o, device->id);
      serializeJson(o, *response);
      request->send(response);
    }
  }

  void handleThingActionDelete(AsyncWebServerRequest *request,
                               ThingDevice *device, ThingAction *action) {
    if (!verifyHost(request)) {
      return;
    }

    String url = request->url();
    String base = "/actions/" + action->id;
    if (url == base || url == base + "/") {
      request->send(404);
      return;
    }

    String actionId = url.substring(base.length() + 1);
    const char *actionIdC = actionId.c_str();
    const char *slash = strchr(actionIdC, '/');

    if (slash) {
      actionId = actionId.substring(0, slash - actionIdC);
    }

    device->removeAction(actionId);
    request->send(204);
  }

  void handleThingActionPost(AsyncWebServerRequest *request,
                             ThingDevice *device, ThingAction *action) {
    if (!verifyHost(request)) {
      return;
    }

    if (!b_has_body_data) {
      request->send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument *newActionBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newActionBuffer, (const char *)body_data);
    if (error) { // unable to parse json
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      request->send(500);
      delete newActionBuffer;
      return;
    }

    ThingActionObject *obj =
        device->requestAction(action->id.c_str(), newActionBuffer);

    if (obj == nullptr) {
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      request->send(500);
      delete newActionBuffer;
      return;
    }

#ifndef WITHOUT_WS
    obj->setNotifyFunction(std::bind(&ThingDevice::sendActionStatus, device,
                                     std::placeholders::_1));
#endif

    DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject item = respBuffer.to<JsonObject>();
    obj->serialize(item, device->id);
    String jsonStr;
    serializeJson(item, jsonStr);
    AsyncWebServerResponse *response =
        request->beginResponse(201, "application/json", jsonStr);
    request->send(response);

    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));

    obj->start();
  }

  void handleThingEventGet(AsyncWebServerRequest *request, ThingDevice *device,
                           ThingItem *item) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue, item->id);
    serializeJson(queue, *response);
    request->send(response);
  }

  void handleThingPropertiesGet(AsyncWebServerRequest *request,
                                ThingItem *rootItem) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject prop = doc.to<JsonObject>();
    ThingItem *item = rootItem;
    while (item != nullptr) {
      item->serializeValueToObject(prop);
      item = item->next;
    }
    serializeJson(prop, *response);
    request->send(response);
  }

  void handleThingActionsGet(AsyncWebServerRequest *request,
                             ThingDevice *device) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeActionQueue(queue);
    serializeJson(queue, *response);
    request->send(response);
  }

  void handleThingEventsGet(AsyncWebServerRequest *request,
                            ThingDevice *device) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue);
    serializeJson(queue, *response);
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

  void handleThingPropertyPut(AsyncWebServerRequest *request,
                              ThingDevice *device, ThingProperty *property) {
    if (!verifyHost(request)) {
      return;
    }
    if (!b_has_body_data) {
      request->send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(newBuffer, body_data);
    if (error) { // unable to parse json
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      request->send(500);
      return;
    }
    JsonVariant newProp = newBuffer.as<JsonVariant>();

    device->setProperty(property->id.c_str(), newProp);

    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    serializeJson(newProp, *response);
    request->send(response);

    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));
  }
};

#endif // ESP
