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

#if defined(SEEED_WIO_TERMINAL)

#include <ArduinoJson.h>
#include <Seeed_atmDNS.h>
#include <WebServer.h>
#include <WiFiClient.h>
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
  WebThingAdapter(String _name, IPAddress _ip, uint16_t _port = 80)
      : server(_port), name(_name), ip(_ip.toString()), port(_port) {}

  void begin() {
    name.toLowerCase();
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("webthing", "tcp", port);
    MDNS.addServiceTxt("webthing", "tcp", "path", "/");

    this->server.onNotFound(std::bind(&WebThingAdapter::handleUnknown, this));

    this->server.on("/*", HTTP_OPTIONS,
                    std::bind(&WebThingAdapter::handleOptions, this));

    this->server.on("/", HTTP_GET,
                    std::bind(&WebThingAdapter::handleThings, this));

    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      String deviceBase = "/things/" + device->id;

      ThingProperty *property = device->firstProperty;
      while (property != nullptr) {
        String propertyBase = deviceBase + "/properties/" + property->id;
        this->server.on(propertyBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingPropertyGet,
                                  this, property));
        this->server.on(propertyBase.c_str(), HTTP_PUT,
                        std::bind(&WebThingAdapter::handleThingPropertyPut,
                                  this, device, property),
                        std::bind(&WebThingAdapter::handleBody, this));

        property = (ThingProperty *)property->next;
      }

      ThingAction *action = device->firstAction;
      while (action != nullptr) {
        String actionBase = deviceBase + "/actions/" + action->id;
        this->server.on(actionBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingActionGet, this,
                                  device, action));
        this->server.on(actionBase.c_str(), HTTP_POST,
                        std::bind(&WebThingAdapter::handleThingActionPost,
                                  this, device, action),
                        std::bind(&WebThingAdapter::handleBody, this));
        this->server.on(actionBase.c_str(), HTTP_DELETE,
                        std::bind(&WebThingAdapter::handleThingActionDelete,
                                  this, device, action));
        action = (ThingAction *)action->next;
      }

      ThingEvent *event = device->firstEvent;
      while (event != nullptr) {
        String eventBase = deviceBase + "/events/" + event->id;
        this->server.on(eventBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingEventGet, this,
                                  device, event));
        event = (ThingEvent *)event->next;
      }

      this->server.on((deviceBase + "/properties").c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThingPropertiesGet,
                                this, device->firstProperty));
      this->server.on(
          (deviceBase + "/actions").c_str(), HTTP_GET,
          std::bind(&WebThingAdapter::handleThingActionsGet, this, device));
      this->server.on(
          (deviceBase + "/actions").c_str(), HTTP_POST,
          std::bind(&WebThingAdapter::handleThingActionsPost, this, device),
          std::bind(&WebThingAdapter::handleBody, this));
      this->server.on(
          (deviceBase + "/events").c_str(), HTTP_GET,
          std::bind(&WebThingAdapter::handleThingEventsGet, this, device));
      this->server.on(deviceBase.c_str(), HTTP_GET,
                      std::bind(&WebThingAdapter::handleThing, this, device));

      device = device->next;
    }

    this->server.begin();
  }

  void update() { server.handleClient(); }

  void addDevice(ThingDevice *device) {
    if (this->lastDevice == nullptr) {
      this->firstDevice = device;
      this->lastDevice = device;
    } else {
      this->lastDevice->next = device;
      this->lastDevice = device;
    }
  }

private:
  WebServer server;

  String name;
  String ip;
  uint16_t port;
  ThingDevice *firstDevice = nullptr;
  ThingDevice *lastDevice = nullptr;
  char body_data[HTTP_UPLOAD_BUFLEN];
  size_t body_index = 0;
  bool b_has_body_data = false;

  void sendCORSHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods",
                      "GET, POST, PUT, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers",
                      "Origin, X-Requested-With, Content-Type, Accept");
  }

  bool verifyHost() {
    String host = server.header("Host");
    if (host == "") {
      server.send(403);
      return false;
    }

    int colonIndex = host.indexOf(':');
    if (colonIndex >= 0) {
      host.remove(colonIndex);
    }

    if (host == name + ".local" || host == ip || host == "localhost") {
      return true;
    }

    server.send(403);
    return false;
  }

  void handleUnknown() {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    server.send(404);
  }

  void handleOptions() {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    server.send(204);
  }

  void handleThings() {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray things = buf.to<JsonArray>();
    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      JsonObject descr = things.createNestedObject();
      device->serialize(descr, ip, port);
      descr["href"] = "/things/" + device->id;
      device = device->next;
    }

    String response;
    serializeJson(things, response);
    server.send(200, "application/json", response);
  }

  void handleThing(ThingDevice *&device) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject descr = buf.to<JsonObject>();
    device->serialize(descr, ip, port);

    String response;
    serializeJson(descr, response);
    server.send(200, "application/json", response);
  }

  void handleThingPropertyGet(ThingItem *item) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonVariant variant = doc.to<JsonVariant>();
    item->serializeValueToVariant(variant);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleThingActionGet(ThingDevice *device, ThingAction *action) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    String url = server.uri();
    String base = "/things/" + device->id + "/actions/" + action->id;
    if (url == base || url == base + "/") {
      DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
      JsonArray queue = doc.to<JsonArray>();
      device->serializeActionQueue(queue, action->id);

      String response;
      serializeJson(queue, response);
      server.send(200, "application/json", response);
    } else {
      String actionId = url.substring(base.length() + 1);
      const char *actionIdC = actionId.c_str();
      const char *slash = strchr(actionIdC, '/');

      if (slash) {
        actionId = actionId.substring(0, slash - actionIdC);
      }

      ThingActionObject *obj = device->findActionObject(actionId.c_str());
      if (obj == nullptr) {
        server.send(404);
        return;
      }

      DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
      JsonObject o = doc.to<JsonObject>();
      obj->serialize(o, device->id);

      String response;
      serializeJson(o, response);
      server.send(200, "application/json", response);
    }
  }

  void handleThingActionDelete(ThingDevice *device, ThingAction *action) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    String url = server.uri();
    String base = "/things/" + device->id + "/actions/" + action->id;
    if (url == base || url == base + "/") {
      server.send(404);
      return;
    }

    String actionId = url.substring(base.length() + 1);
    const char *actionIdC = actionId.c_str();
    const char *slash = strchr(actionIdC, '/');

    if (slash) {
      actionId = actionId.substring(0, slash - actionIdC);
    }

    device->removeAction(actionId);
    server.send(204);
  }

  void handleThingActionPost(ThingDevice *device, ThingAction *action) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    if (!b_has_body_data) {
      server.send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newBuffer, (const char *)body_data);
    if (error) { // unable to parse json
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(500);
      delete newBuffer;
      return;
    }

    JsonObject newAction = newBuffer->as<JsonObject>();

    if (!newAction.containsKey(action->id)) {
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(400);
      delete newBuffer;
      return;
    }

    ThingActionObject *obj = device->requestAction(newBuffer);

    if (obj == nullptr) {
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(500);
      delete newBuffer;
      return;
    }

    DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject item = respBuffer.to<JsonObject>();
    obj->serialize(item, device->id);
    String jsonStr;
    serializeJson(item, jsonStr);
    server.send(201, "application/json", jsonStr);

    body_index = 0;
    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));

    obj->start();
  }

  void handleThingEventGet(ThingDevice *device, ThingItem *item) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue, item->id);

    String response;
    serializeJson(queue, response);
    server.send(200, "application/json", response);
  }

  void handleThingPropertiesGet(ThingItem *rootItem) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject prop = doc.to<JsonObject>();
    ThingItem *item = rootItem;
    while (item != nullptr) {
      item->serializeValueToObject(prop);
      item = item->next;
    }

    String response;
    serializeJson(prop, response);
    server.send(200, "application/json", response);
  }

  void handleThingActionsGet(ThingDevice *device) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeActionQueue(queue);

    String response;
    serializeJson(queue, response);
    server.send(200, "application/json", response);
  }

  void handleThingActionsPost(ThingDevice *device) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    if (!b_has_body_data) {
      server.send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newBuffer, (const char *)body_data);
    if (error) { // unable to parse json
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(500);
      delete newBuffer;
      return;
    }

    JsonObject newAction = newBuffer->as<JsonObject>();

    if (newAction.size() != 1) {
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(400);
      delete newBuffer;
      return;
    }

    ThingActionObject *obj = device->requestAction(newBuffer);

    if (obj == nullptr) {
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(500);
      delete newBuffer;
      return;
    }

    DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject item = respBuffer.to<JsonObject>();
    obj->serialize(item, device->id);
    String jsonStr;
    serializeJson(item, jsonStr);
    server.send(201, "application/json", jsonStr);

    body_index = 0;
    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));

    obj->start();
  }

  void handleThingEventsGet(ThingDevice *device) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue);

    String response;
    serializeJson(queue, response);
    server.send(200, "application/json", response);
  }

  void handleBody() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_WRITE) {
      // copy to internal buffer
      memcpy(&body_data[body_index], upload.buf, upload.currentSize);
      b_has_body_data = true;
      body_index += upload.currentSize;
    }
  }

  void handleThingPropertyPut(ThingDevice *device, ThingProperty *property) {
    sendCORSHeaders();
    if (!verifyHost()) {
      return;
    }

    if (!b_has_body_data) {
      server.send(422); // unprocessable entity (b/c no body)
      return;
    }

    DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(newBuffer, body_data);
    if (error) { // unable to parse json
      body_index = 0;
      b_has_body_data = false;
      memset(body_data, 0, sizeof(body_data));
      server.send(500);
      return;
    }
    JsonVariant newProp = newBuffer.as<JsonVariant>();

    device->setProperty(property->id.c_str(), newProp);

    String response;
    serializeJson(newProp, response);
    server.send(200, "application/json", response);

    body_index = 0;
    b_has_body_data = false;
    memset(body_data, 0, sizeof(body_data));
  }
};

#endif // SEEED
