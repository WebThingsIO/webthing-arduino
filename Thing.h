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

#ifndef MOZILLA_IOT_THING_H
#define MOZILLA_IOT_THING_H

#if !defined(WITHOUT_WS) && (defined(ESP8266) || defined(ESP32))
#include <ESPAsyncWebServer.h>
#endif

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
};

#endif
