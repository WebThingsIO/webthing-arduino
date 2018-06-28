/**
 * ESP8266WebThingAdapter.h
 *
 * Exposes the Web Thing API based on provided ThingDevices.
 * Suitable for ESP8266 with built-in Webserver and mDNS.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MOZILLA_IOT_ESP8266WEBTHINGADAPTER_H
#define MOZILLA_IOT_ESP8266WEBTHINGADAPTER_H

#ifdef ESP8266

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Thing.h"

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

    this->server.on("/", [this]() {
      this->handleThings();
    });
    ThingDevice* device = this->firstDevice;
    while (device != nullptr) {
      String deviceBase = "/things/" + device->id;
      server.on(deviceBase, [this, device]() {
        this->handleThing(device);
      });

      ThingProperty* property = device->firstProperty;
      while (property != nullptr) {
        String propertyBase = deviceBase + "/properties/" + property->id;
        server.on(propertyBase, HTTP_GET, [this, property]() {
          this->handleThingPropertyGet(property);
        });
        server.on(propertyBase, HTTP_PUT, [this, property]() {
          this->handleThingPropertyPut(property);
        });
        property = property->next;
      }
      device = device->next;
    }

    this->server.begin();
  }

  void update() {
    this->server.handleClient();
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
  ESP8266WebServer server;
  String name;
  ThingDevice* firstDevice = nullptr;
  ThingDevice* lastDevice = nullptr;

  void sendCORS() {
    this->server.sendHeader("Access-Control-Allow-Origin", "*", false);
    this->server.sendHeader("Access-Control-Allow-Methods", "PUT, GET, OPTIONS", false);
  }

  void handleThings() {
    StaticJsonBuffer<2048> buf;
    JsonArray& things = buf.createArray();
    ThingDevice* device = this->firstDevice;
    while (device != nullptr) {
      JsonObject& descr = things.createNestedObject();
      this->serializeDevice(descr, device);
      device = device->next;
    }

    String strBuf;
    things.printTo(strBuf);

    this->sendCORS();
    this->server.send(200, "application/json", strBuf);
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


  void handleThing(ThingDevice* device) {
    StaticJsonBuffer<1024> buf;
    JsonObject& descr = buf.createObject();
    this->serializeDevice(descr, device);

    String strBuf;
    descr.printTo(strBuf);

    this->sendCORS();
    this->server.send(200, "application/json", strBuf);
  }

  void handleThingPropertyGet(ThingProperty* property) {
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
    String strBuf;
    prop.printTo(strBuf);

    this->sendCORS();
    this->server.send(200, "application/json", strBuf);
  }

  void handleThingPropertyPut(ThingProperty* property) {
    StaticJsonBuffer<256> newBuffer;
    JsonObject& newProp = newBuffer.parseObject(this->server.arg("plain"));
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

    this->sendCORS();
    this->server.send(200, "application/json", server.arg("plain"));
  }
};

#endif    // ESP8266

#endif
