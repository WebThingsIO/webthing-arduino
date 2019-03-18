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
  WebThingAdapter(String _name, IPAddress _ip): name(_name), server(80), ip(_ip.toString()) {
  }

  void begin() {
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("webthing", "tcp", 80);
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
        this->server.on(propertyBase.c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThingPropertyGet, this, std::placeholders::_1, property));
        this->server.on(propertyBase.c_str(), HTTP_PUT,
          std::bind(&WebThingAdapter::handleThingPropertyPut, this, std::placeholders::_1, property),
          NULL,
          std::bind(&WebThingAdapter::handleBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
        );

        property = property->next;
      }

      this->server.on(deviceBase.c_str(), HTTP_GET, std::bind(&WebThingAdapter::handleThing, this, std::placeholders::_1, device));

      device = device->next;

    }

    this->server.begin();
  }

  void update() {
    MDNS.update();
  }

  void addDevice(ThingDevice* device) {
    if (this->lastDevice == nullptr) {
      this->firstDevice = device;
      this->lastDevice = device;
    } else {
      this->lastDevice->next = device;
      this->lastDevice = device;
    }
  }

private:
  AsyncWebServer server;
  String name;
  String ip;
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

    DynamicJsonBuffer buf(4096);
    JsonArray& things = buf.createArray();
    ThingDevice* device = this->firstDevice;
    while (device != nullptr) {
      JsonObject& descr = things.createNestedObject();
      this->serializeDevice(descr, device);
      device = device->next;
    }

    things.printTo(*response);
    request->send(response);

  }

  void serializeDevice(JsonObject& descr, ThingDevice* device) {
    descr["name"] = device->name;
    descr["href"] = "/things/" + device->id;
    descr["@context"] = "https://iot.mozilla.org/schemas";

    JsonArray& typeJson = descr.createNestedArray("@type");
    const char** type = device->type;
    while ((*type) != nullptr) {
      typeJson.add(*type);
      type++;
    }

    JsonObject& props = descr.createNestedObject("properties");

    ThingProperty* property = device->firstProperty;
    while (property != nullptr) {
      JsonObject& prop = props.createNestedObject(property->id);
      switch (property->type) {
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

      if (property->readOnly) {
        prop["readOnly"] = true;
      }

      if (property->unit != "") {
        prop["unit"] = property->unit;
      }

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

      if (property->atType != nullptr) {
        prop["@type"] = property->atType;
      }
      prop["href"] = "/things/" + device->id + "/properties/" + property->id;
      property = property->next;
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

  void handleThingPropertyGet(AsyncWebServerRequest *request, ThingProperty* property) {
    if (!verifyHost(request)) {
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonBuffer buf(256);
    JsonObject& prop = buf.createObject();
    switch (property->type) {
    case BOOLEAN:
      prop[property->id] = property->getValue().boolean;
      break;
    case NUMBER:
      prop[property->id] = property->getValue().number;
      break;
    case STRING:
      prop[property->id] = *property->getValue().string;
      break;
    }

    prop.printTo(*response);
    request->send(response);
  }


  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if ( total >= ESP_MAX_PUT_BODY_SIZE || index+len >= ESP_MAX_PUT_BODY_SIZE) {
        return; // cannot store this size..
    }
    // copy to internal buffer
    memcpy(&body_data[index], data, len);
    b_has_body_data = true;
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
    if (newProp.success()) {
      JsonVariant newValue = newProp[property->id];

      switch (property->type) {
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
        *property->getValue().string = newValue.as<String>();
        break;
      }

      AsyncResponseStream *response = request->beginResponseStream("application/json");
      newProp.printTo(*response);
      request->send(response);
    } else {
      // unable to parse json
      request->send(500);
    }
    b_has_body_data = false;
    memset(body_data,0,sizeof(body_data));
  }

};

#endif    // ESP

#endif
