/**
 * WiFi101WebThingAdapter.h
 *
 * Exposes the Web Thing API based on provided ThingDevices.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if !defined(ESP32) && !defined(ESP8266) && !defined(SEEED_WIO_TERMINAL)

#include <Arduino.h>

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#include <WiFiNINA.h>
#else
#include <WiFi101.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoMDNS.h>

#include <ArduinoJson.h>

#define WITHOUT_WS 1
#include "Thing.h"

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

static const bool DEBUG = false;

enum HTTPMethod {
  HTTP_ANY,
  HTTP_GET,
  HTTP_PUT,
  HTTP_POST,
  HTTP_DELETE,
  HTTP_OPTIONS
};

enum ReadState {
  STATE_READ_METHOD,
  STATE_READ_URI,
  STATE_DISCARD_HTTP11,
  STATE_DISCARD_HEADERS_PRE_HOST,
  STATE_READ_HOST,
  STATE_DISCARD_HEADERS_POST_HOST,
  STATE_READ_CONTENT
};

class WebThingAdapter {
public:
  WebThingAdapter(ThingDevice *_thing, String _name, uint32_t _ip, uint16_t _port = 80)
      : thing(_thing), name(_name), port(_port), server(_port), mdns(udp) {
    ip = "";
    for (int i = 0; i < 4; i++) {
      ip += _ip & 0xff;
      if (i < 3) {
        ip += '.';
      }
      _ip >>= 8;
    }
  }

  void begin() {
    name.toLowerCase();

    String serviceName = name + "._webthing";
    mdns.begin(WiFi.localIP(), name.c_str());
    // \x06 is the length of the record
    mdns.addServiceRecord(serviceName.c_str(), port, MDNSServiceTCP,
                          "\x06path=/");

    server.begin();
  }

  void update() {
    mdns.run();

    if (!client) {
      WiFiClient client = server.available();
      if (!client) {
        return;
      }
      if (DEBUG) {
        Serial.println("New client available");
      }
      this->client = client;
    }

    if (!client.connected()) {
      if (DEBUG) {
        Serial.println("Client disconnected");
      }
      resetParser();
      client.stop();
      return;
    }

    char c = client.read();
    if (c == 255 || c == -1) {
      if (state == STATE_READ_CONTENT) {
        handleRequest();
        resetParser();
      }

      retries += 1;
      if (retries > 5000) {
        if (DEBUG) {
          Serial.println("Giving up on client");
        }
        resetParser();
        client.stop();
      }
      return;
    }

    switch (state) {
    case STATE_READ_METHOD:
      if (c == ' ') {
        if (methodRaw == "GET") {
          method = HTTP_GET;
        } else if (methodRaw == "POST") {
          method = HTTP_POST;
        } else if (methodRaw == "PUT") {
          method = HTTP_PUT;
        } else if (methodRaw == "DELETE") {
          method = HTTP_DELETE;
        } else if (methodRaw == "OPTIONS") {
          method = HTTP_OPTIONS;
        } else {
          method = HTTP_ANY;
        }
        state = STATE_READ_URI;
      } else {
        methodRaw += c;
      }
      break;

    case STATE_READ_URI:
      if (c == ' ') {
        state = STATE_DISCARD_HTTP11;
      } else {
        uri += c;
      }
      break;

    case STATE_DISCARD_HTTP11:
      if (c == '\r') {
        state = STATE_DISCARD_HEADERS_PRE_HOST;
      }
      break;

    case STATE_DISCARD_HEADERS_PRE_HOST:
      if (c == '\r') {
        break;
      }
      if (c == '\n') {
        headerRaw = "";
        break;
      }
      if (c == ':') {
        if (headerRaw.equalsIgnoreCase("Host")) {
          state = STATE_READ_HOST;
        }
        break;
      }

      headerRaw += c;
      break;

    case STATE_READ_HOST:
      if (c == '\r') {
        returnsAndNewlines = 1;
        state = STATE_DISCARD_HEADERS_POST_HOST;
        break;
      }
      if (c == ' ') {
        break;
      }
      host += c;
      break;

    case STATE_DISCARD_HEADERS_POST_HOST:
      if (c == '\r' || c == '\n') {
        returnsAndNewlines += 1;
      } else {
        returnsAndNewlines = 0;
      }
      if (returnsAndNewlines == 4) {
        state = STATE_READ_CONTENT;
      }
      break;

    case STATE_READ_CONTENT:
      content += c;
      break;
    }
  }

private:
  ThingDevice *thing = nullptr;
  String name, ip;
  uint16_t port;
  WiFiServer server;
  WiFiClient client;
  WiFiUDP udp;
  MDNS mdns;

  ReadState state = STATE_READ_METHOD;
  String uri = "";
  HTTPMethod method = HTTP_ANY;
  String content = "";
  String methodRaw = "";
  String host = "";
  String headerRaw = "";
  int returnsAndNewlines = 0;
  int retries = 0;

  ThingDevice *firstDevice = nullptr, *lastDevice = nullptr;

  bool verifyHost() {
    int colonIndex = host.indexOf(':');
    if (colonIndex >= 0) {
      host.remove(colonIndex);
    }
    if (host == name + ".local") {
      return true;
    }
    if (host == ip) {
      return true;
    }
    if (host == "localhost") {
      return true;
    }
    return false;
  }

  void handleRequest() {
    if (DEBUG) {
      Serial.print("handleRequest: ");
      Serial.print("method: ");
      Serial.println(method);
      Serial.print("uri: ");
      Serial.println(uri);
      Serial.print("host: ");
      Serial.println(host);
      Serial.print("content: ");
      Serial.println(content);
    }

    if (!verifyHost()) {
      client.println("HTTP/1.1 403 Forbidden");
      client.println("Connection: close");
      client.println();
      delay(1);
      client.stop();
      return;
    }

    ThingDevice *device = this->thing;

    if (uri == "/") {
      if (method == HTTP_GET || method == HTTP_OPTIONS) {
        handleThing(device);
      } else {
        handleError();
      }
      return;
    } else if (uri == "/properties") {
      if (method == HTTP_GET || method == HTTP_OPTIONS) {
        handleThingPropertiesGet(device->firstProperty);
      } else {
        handleError();
      }
      return;
    } else if (uri == "/actions") {
      if (method == HTTP_GET || method == HTTP_OPTIONS) {
        handleThingActionsGet(device);
      } else {
        handleError();
      }
      return;
    } else if (uri == "/events") {
      if (method == HTTP_GET || method == HTTP_OPTIONS) {
        handleThingEventsGet(device);
      } else {
        handleError();
      }
      return;
    } else {
      ThingProperty *property = device->firstProperty;
      while (property != nullptr) {
        String propertyBase = "/properties/" + property->id;
        if (uri == propertyBase) {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingPropertyGet(property);
          } else if (method == HTTP_PUT) {
            handleThingPropertyPut(device, property);
          } else {
            handleError();
          }
          return;
        }
        property = (ThingProperty *)property->next;
      }

      ThingAction *action = device->firstAction;
      while (action != nullptr) {
        String actionBase = "/actions/" + action->id;
        if (uri == actionBase) {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingActionGet(device, action);
          } else if (method == HTTP_POST) {
            handleThingActionPost(device, action);
          } else {
            handleError();
          }
          return;
        } else if (uri.startsWith(actionBase + "/") &&
                    uri.length() > (actionBase.length() + 1)) {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingActionIdGet(device, action);
          } else if (method == HTTP_DELETE) {
            handleThingActionIdDelete(device, action);
          } else {
            handleError();
          }
          return;
        }
        action = action->next;
      }

      ThingEvent *event = device->firstEvent;
      while (event != nullptr) {
        String eventBase = "/events/" + event->id;
        if (uri == eventBase) {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingEventGet(device, event);
          } else {
            handleError();
          }
          return;
        }
        event = (ThingEvent *)event->next;
      }
    }
    handleError();
  }

  void sendOk() { client.println("HTTP/1.1 200 OK"); }

  void sendCreated() { client.println("HTTP/1.1 201 Created"); }

  void sendNoContent() { client.println("HTTP/1.1 204 No Content"); }

  void sendHeaders() {
    client.println("Access-Control-Allow-Origin: *");
    client.println(
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS");
    client.println("Access-Control-Allow-Headers: "
                   "Origin, X-Requested-With, Content-Type, Accept");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
  }

  void handleThing(ThingDevice *device) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject descr = buf.to<JsonObject>();
    device->serialize(descr, ip, port);

    serializeJson(descr, client);
    delay(1);
    client.stop();
  }

  void handleThingPropertyGet(ThingItem *item) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonVariant variant = doc.to<JsonVariant>();
    item->serializeValueToVariant(variant);
    serializeJson(doc, client);
    delay(1);
    client.stop();
  }

  void handleThingActionGet(ThingDevice *device, ThingAction *action) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeActionQueue(queue, action->id);
    serializeJson(queue, client);
    delay(1);
    client.stop();
  }

  void handleThingActionIdGet(ThingDevice *device, ThingAction *action) {
    String base = "/things/" + device->id + "/actions/" + action->id;
    String actionId = uri.substring(base.length() + 1);
    const char *actionIdC = actionId.c_str();
    const char *slash = strchr(actionIdC, '/');

    if (slash) {
      actionId = actionId.substring(0, slash - actionIdC);
    }

    ThingActionObject *obj = device->findActionObject(actionId.c_str());
    if (obj == nullptr) {
      handleError();
      return;
    }

    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject o = doc.to<JsonObject>();
    obj->serialize(o, device->id);
    serializeJson(o, client);
    delay(1);
    client.stop();
  }

  void handleThingActionIdDelete(ThingDevice *device, ThingAction *action) {
    String base = "/things/" + device->id + "/actions/" + action->id;
    String actionId = uri.substring(base.length() + 1);
    const char *actionIdC = actionId.c_str();
    const char *slash = strchr(actionIdC, '/');

    if (slash) {
      actionId = actionId.substring(0, slash - actionIdC);
    }

    device->removeAction(actionId);
    sendNoContent();
    sendHeaders();
  }

  void handleThingActionPost(ThingDevice *device, ThingAction *action) {
    DynamicJsonDocument *newActionBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newActionBuffer, content);
    if (error) { // unable to parse json
      handleError();
      delete newActionBuffer;
      return;
    }

    ThingActionObject *obj = device->requestAction(action->id.c_str(), newActionBuffer);

    if (obj == nullptr) {
      handleError();
      delete newActionBuffer;
      return;
    }

    sendCreated();
    sendHeaders();

    DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject item = respBuffer.to<JsonObject>();
    obj->serialize(item, device->id);
    serializeJson(item, client);
    delay(1);
    client.stop();

    obj->start();
  }

  void handleThingEventGet(ThingDevice *device, ThingItem *item) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue, item->id);
    serializeJson(queue, client);
    delay(1);
    client.stop();
  }

  void handleThingPropertiesGet(ThingItem *rootItem) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject prop = doc.to<JsonObject>();
    ThingItem *item = rootItem;
    while (item != nullptr) {
      item->serializeValueToObject(prop);
      item = item->next;
    }
    serializeJson(prop, client);
    delay(1);
    client.stop();
  }

  void handleThingActionsGet(ThingDevice *device) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeActionQueue(queue);
    serializeJson(queue, client);
    delay(1);
    client.stop();
  }

  void handleThingEventsGet(ThingDevice *device) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray queue = doc.to<JsonArray>();
    device->serializeEventQueue(queue);
    serializeJson(queue, client);
    delay(1);
    client.stop();
  }

  void handleThingPropertyPut(ThingDevice *device, ThingProperty *property) {
    DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(newBuffer, content);
    if (error) { // unable to parse json
      handleError();
      return;
    }
    JsonVariant newProp = newBuffer.as<JsonVariant>();

    device->setProperty(property->id.c_str(), newProp);

    sendOk();
    sendHeaders();

    serializeJson(newProp, client);
    delay(1);
    client.stop();
  }

  void handleError() {
    client.println("HTTP/1.1 400 Bad Request");
    sendHeaders();
    delay(1);
    client.stop();
  }

  void resetParser() {
    state = STATE_READ_METHOD;
    method = HTTP_ANY;
    methodRaw = "";
    headerRaw = "";
    host = "";
    uri = "";
    content = "";
    retries = 0;
  }
};

#endif // neither ESP32 nor ESP8266 defined
