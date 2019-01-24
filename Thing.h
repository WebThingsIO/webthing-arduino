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

enum ThingPropertyType {
  BOOLEAN,
  NUMBER,
  STRING
};

union ThingPropertyValue {
  bool boolean;
  double number;
  String* string;
};

class ThingProperty {
public:
  String id;
  String description;
  ThingPropertyType type;
  String atType;
  ThingProperty* next = nullptr;

  bool readOnly = false;
  const char** propertyEnum = nullptr;
  String unit="";

  ThingProperty(const char* id_, const char* description_, ThingPropertyType type_, const char* atType_):
    id(id_),
    description(description_),
    type(type_),
    atType(atType_) {
  }

  void setValue(ThingPropertyValue newValue) {
    this->value = newValue;
  }

  ThingPropertyValue getValue() {
    return this->value;
  }

private:
  ThingPropertyValue value = {false};
};

class ThingDevice {
public:
  String id;
  String name;
  const char** type;
  ThingDevice* next = nullptr;
  ThingProperty* firstProperty = nullptr;
  ThingProperty* lastProperty = nullptr;

  ThingDevice(const char* _id, const char* _name, const char** _type):
    id(_id),
    name(_name),
    type(_type) {
  }

  void addProperty(ThingProperty* property) {
    if (lastProperty == nullptr) {
      firstProperty = property;
      lastProperty = property;
    } else {
      lastProperty->next = property;
      lastProperty = property;
    }
  }
};

#endif
