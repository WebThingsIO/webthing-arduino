/**
 * ESP32WebThingAdapter.h
 *
 * Exposes the Web Thing API based on provided ThingDevices.
 * Suitable for ESP32, ESPAsyncWebServer, ESPAsyncTCP
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MOZILLA_IOT_ESP32WEBTHINGADAPTER_H
#define MOZILLA_IOT_ESP32WEBTHINGADAPTER_H

#ifdef ESP32

#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "Thing.h"

#define ESP32_MAX_PUT_BODY_SIZE 256


class WebThingAdapter {
public:
  WebThingAdapter(String _name): name(_name), server(80) {
  }

  void begin() {
    if (MDNS.begin(this->name.c_str())) {
      Serial.println("MDNS responder started");
    }

    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "url",
        "http://" + this->name + ".local");
    MDNS.addServiceTxt("http", "tcp", "webthing", "true");

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
    // non implemented, async web-server
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
  ThingDevice* firstDevice = nullptr;
  ThingDevice* lastDevice = nullptr;
  char body_data[ESP32_MAX_PUT_BODY_SIZE];
  bool b_has_body_data = false;

  void handleUnknown(AsyncWebServerRequest *request) {
    request->send(404);
  }

  void handleThings(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonBuffer<2048> buf;
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
      prop["@type"] = property->atType;
      prop["href"] = "/things/" + device->id + "/properties/" + property->id;
      property = property->next;
    }
  }

  void handleThing(AsyncWebServerRequest *request, ThingDevice*& device) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonBuffer<1024> buf;
    JsonObject& descr = buf.createObject();
    this->serializeDevice(descr, device);

    descr.printTo(*response);
    request->send(response);
  }

  void handleThingPropertyGet(AsyncWebServerRequest *request, ThingProperty* property) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonBuffer<256> buf;
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
    if ( total >= ESP32_MAX_PUT_BODY_SIZE || index+len >= ESP32_MAX_PUT_BODY_SIZE) {
        return; // cannot store this size..
    }
    // copy to internal buffer
    memcpy(&body_data[index], data, len);
    b_has_body_data = true;
  }

  void handleThingPropertyPut(AsyncWebServerRequest *request, ThingProperty* property) {
    if (!b_has_body_data) {
      request->send(422); // unprocessable entity (b/c no body)
      return;
    }

    StaticJsonBuffer<256> newBuffer;
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

#endif    // ESP32

#endif
