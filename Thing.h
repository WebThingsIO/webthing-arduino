/**
 * Thing.h
 *
 * Provides ThingProperty and ThingDevice classes for creating modular Web
 * Things.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if !defined(ESP8266) && !defined(ESP32) && !defined(WITHOUT_WS)
#define WITHOUT_WS 1
#endif

#if !defined(WITHOUT_WS) && (defined(ESP8266) || defined(ESP32))
#include <ESPAsyncWebServer.h>
#endif

#include <ArduinoJson.h>

enum ThingPropertyType { NO_STATE, BOOLEAN, NUMBER, INTEGER, STRING };

union ThingPropertyValue {
  bool boolean;
  double number;
  signed long long integer;
  String *string;
};

class ThingItem {
public:
  String id;
  String description;
  ThingPropertyType type;
  String atType;
  ThingItem *next = nullptr;

  bool readOnly = false;
  String unit = "";
  String title = "";
  double minimum = 0;
  double maximum = -1;
  double multipleOf = -1;

  ThingItem(const char *id_, const char *description_, ThingPropertyType type_,
            const char *atType_)
      : id(id_), description(description_), type(type_), atType(atType_) {}

  void setValue(ThingPropertyValue newValue) {
    this->value = newValue;
    this->hasChanged = true;
  }

  /**
   * Returns the property value if it has been changed via {@link setValue}
   * since the last call or returns a nullptr.
   */
  ThingPropertyValue *changedValueOrNull() {
    ThingPropertyValue *v = this->hasChanged ? &this->value : nullptr;
    this->hasChanged = false;
    return v;
  }

  ThingPropertyValue getValue() { return this->value; }

  void serialize(JsonObject prop) {
    switch (this->type) {
    case NO_STATE:
      break;
    case BOOLEAN:
      prop[this->id] = this->getValue().boolean;
      break;
    case NUMBER:
      prop[this->id] = this->getValue().number;
      break;
    case INTEGER:
      prop[this->id] = this->getValue().integer;
      break;
    case STRING:
      prop[this->id] = *this->getValue().string;
      break;
    }
  }

private:
  ThingPropertyValue value = {false};
  bool hasChanged = false;
};

class ThingProperty : public ThingItem {
public:
  const char **propertyEnum = nullptr;

  ThingProperty(const char *id_, const char *description_,
                ThingPropertyType type_, const char *atType_)
      : ThingItem(id_, description_, type_, atType_) {}
};

using ThingEvent = ThingItem;

class ThingDevice {
public:
  String id;
  String title;
  String description;
  const char **type;
#if !defined(WITHOUT_WS) && (defined(ESP8266) || defined(ESP32))
  AsyncWebSocket *ws = nullptr;
#endif
  ThingDevice *next = nullptr;
  ThingProperty *firstProperty = nullptr;
  ThingEvent *firstEvent = nullptr;

  ThingDevice(const char *_id, const char *_title, const char **_type)
      : id(_id), title(_title), type(_type) {}

  ~ThingDevice() {
#if !defined(WITHOUT_WS) && (defined(ESP8266) || defined(ESP32))
    if (ws)
      delete ws;
#endif
  }

  ThingProperty *findProperty(const char *id) {
    ThingProperty *p = this->firstProperty;
    while (p) {
      if (!strcmp(p->id.c_str(), id))
        return p;
      p = (ThingProperty *)p->next;
    }
    return nullptr;
  }

  void addProperty(ThingProperty *property) {
    property->next = firstProperty;
    firstProperty = property;
  }

  void addEvent(ThingEvent *event) {
    event->next = firstEvent;
    firstEvent = event;
  }

  void serialize(JsonObject descr
#ifndef WITHOUT_WS
                 ,
                 String ip, uint16_t port
#endif
  ) {
    descr["id"] = this->id;
    descr["title"] = this->title;
    descr["@context"] = "https://iot.mozilla.org/schemas";

    if (this->description != "") {
      descr["description"] = this->description;
    }
    // TODO: descr["base"] = ???

    JsonObject securityDefinitions =
        descr.createNestedObject("securityDefinitions");
    JsonObject nosecSc = securityDefinitions.createNestedObject("nosec_sc");
    nosecSc["scheme"] = "nosec";
    descr["security"] = "nosec_sc";

    JsonArray typeJson = descr.createNestedArray("@type");
    const char **type = this->type;
    while ((*type) != nullptr) {
      typeJson.add(*type);
      type++;
    }

    JsonArray links = descr.createNestedArray("links");
    {
      JsonObject links_prop = links.createNestedObject();
      links_prop["rel"] = "properties";
      links_prop["href"] = "/things/" + this->id + "/properties";
    }

    {
      JsonObject links_prop = links.createNestedObject();
      links_prop["rel"] = "events";
      links_prop["href"] = "/things/" + this->id + "/events";
    }

#ifndef WITHOUT_WS
    {
      JsonObject links_prop = links.createNestedObject();
      links_prop["rel"] = "alternate";
      char buffer[33];
      itoa(port, buffer, 10);
      links_prop["href"] = "ws://" + ip + ":" + buffer + "/things/" + this->id;
    }
#endif

    ThingProperty *property = this->firstProperty;
    if (property) {
      serializePropertyOrEvent(descr, "properties", true, property);
    }

    ThingEvent *event = this->firstEvent;
    if (event) {
      serializePropertyOrEvent(descr, "events", false, event);
    }
  }

  void serializePropertyOrEvent(JsonObject descr, const char *type,
                                bool isProp, ThingItem *item) {
    String basePath = "/things/" + this->id + "/" + type + "/";
    JsonObject props = descr.createNestedObject(type);
    while (item != nullptr) {
      JsonObject prop = props.createNestedObject(item->id);
      switch (item->type) {
      case NO_STATE:
        break;
      case BOOLEAN:
        prop["type"] = "boolean";
        break;
      case NUMBER:
        prop["type"] = "number";
        break;
      case INTEGER:
        prop["type"] = "integer";
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

      if (item->description != "") {
        prop["description"] = item->description;
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
        ThingProperty *property = (ThingProperty *)item;
        const char **enumVal = property->propertyEnum;
        bool hasEnum = (property->propertyEnum != nullptr) &&
                       ((*property->propertyEnum) != nullptr);

        if (hasEnum) {
          enumVal = property->propertyEnum;
          JsonArray propEnum = prop.createNestedArray("enum");
          while (property->propertyEnum != nullptr && (*enumVal) != nullptr) {
            propEnum.add(*enumVal);
            enumVal++;
          }
        }
      }

      if (item->atType != nullptr) {
        prop["@type"] = item->atType;
      }

      // 2.9 Property object: A links array (An array of Link objects linking
      // to one or more representations of a Property resource, each with an
      // implied default rel=property.)
      JsonArray inline_links = prop.createNestedArray("links");
      JsonObject inline_links_prop = inline_links.createNestedObject();
      inline_links_prop["href"] = basePath + item->id;

      item = item->next;
    }
  }
};
