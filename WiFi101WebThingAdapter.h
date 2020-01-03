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

#if !defined(ESP32) && !defined(ESP8266)

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

enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_PUT, HTTP_POST, HTTP_OPTIONS };

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
  WebThingAdapter(String _name, uint32_t _ip, uint16_t _port = 80)
      : name(_name), port(_port), server(_port), mdns(udp) {
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

    mdns.begin(WiFi.localIP(), name.c_str());

    mdns.addServiceRecord("_webthing", port, MDNSServiceTCP, "\x06path=/");

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

    if (uri == "/") {
      handleThings();
      return;
    }

    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      String deviceBase = "/things/" + device->id;

      if (uri.startsWith(deviceBase)) {
        if (uri == deviceBase) {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThing(device);
          } else {
            handleError();
          }
          return;
        } else if (uri == deviceBase + "/properties") {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingPropertiesGet(device->firstProperty);
          } else {
            handleError();
          }
          return;
        } else if (uri == deviceBase + "/actions") {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingActionsGet(device);
          } else if (method == HTTP_POST) {
            handleThingActionsPost(device);
          } else {
            handleError();
          }
          return;
        } else if (uri == deviceBase + "/events") {
          if (method == HTTP_GET || method == HTTP_OPTIONS) {
            handleThingEventsGet(device);
          } else {
            handleError();
          }
          return;
        } else {
          ThingProperty *property = device->firstProperty;
          while (property != nullptr) {
            String propertyBase = deviceBase + "/properties/" + property->id;
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
            String actionBase = deviceBase + "/actions/" + action->id;
            if (uri == actionBase) {
              if (method == HTTP_GET || method == HTTP_OPTIONS) {
                handleThingActionGet(device, action);
              } else if (method == HTTP_POST) {
                handleThingActionPost(device, action);
              } else {
                handleError();
              }
              return;
            }
            action = action->next;
          }

          ThingEvent *event = device->firstEvent;
          while (event != nullptr) {
            String eventBase = deviceBase + "/events/" + event->id;
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
      }
      device = device->next;
    }
    handleError();
  }

  void sendOk() { client.println("HTTP/1.1 200 OK"); }

  void sendCreated() { client.println("HTTP/1.1 201 Created"); }

  void sendHeaders() {
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
  }

  void handleThings() {
    sendOk();
    sendHeaders();

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonArray things = buf.to<JsonArray>();
    ThingDevice *device = this->firstDevice;
    while (device != nullptr) {
      JsonObject descr = things.createNestedObject();
      device->serialize(descr);
      descr["href"] = "/things/" + device->id;
      device = device->next;
    }

    serializeJson(things, client);
    delay(1);
    client.stop();
  }

  void handleThing(ThingDevice *device) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
    JsonObject descr = buf.to<JsonObject>();
    device->serialize(descr);

    serializeJson(descr, client);
    delay(1);
    client.stop();
  }

  void handleThingPropertyGet(ThingItem *item) {
    sendOk();
    sendHeaders();

    DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
    JsonObject prop = doc.to<JsonObject>();
    item->serializeValue(prop);
    serializeJson(prop, client);
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

  void handleThingActionPost(ThingDevice *device, ThingAction *action) {
    DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newBuffer, content);
    if (error) { // unable to parse json
      handleError();
      delete newBuffer;
      return;
    }

    JsonObject newAction = newBuffer->as<JsonObject>();

    if (!newAction.containsKey(action->id)) {
      handleError();
      delete newBuffer;
      return;
    }

    ThingActionObject *obj = device->requestAction(newBuffer);

    if (obj == nullptr) {
      handleError();
      delete newBuffer;
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
      item->serializeValue(prop);
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

  void handleThingActionsPost(ThingDevice *device) {
    DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
    auto error = deserializeJson(*newBuffer, content);
    if (error) { // unable to parse json
      handleError();
      delete newBuffer;
      return;
    }

    JsonObject newAction = newBuffer->as<JsonObject>();

    if (!newAction.size() != 1) {
      handleError();
      delete newBuffer;
      return;
    }

    ThingActionObject *obj = device->requestAction(newBuffer);

    if (obj == nullptr) {
      handleError();
      delete newBuffer;
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
    JsonObject newProp = newBuffer.as<JsonObject>();

    if (!newProp.containsKey(property->id)) {
      handleError();
      return;
    }

    device->setProperty(property->id.c_str(), newProp[property->id]);

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
