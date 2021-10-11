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

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

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

enum ThingDataType { NO_STATE, BOOLEAN, NUMBER, INTEGER, STRING };
typedef ThingDataType ThingPropertyType;

union ThingDataValue {
  bool boolean;
  double number;
  signed long long integer;
  String *string;
};
typedef ThingDataValue ThingPropertyValue;

class ThingActionObject {
private:
  void (*start_fn)(const JsonVariant &);
  void (*cancel_fn)();

public:
  String name;
  DynamicJsonDocument *actionRequest = nullptr;
  String timeRequested;
  String timeCompleted;
  String status;
  String id;
  ThingActionObject *next = nullptr;

  ThingActionObject(const char *name_, DynamicJsonDocument *actionRequest_,
                    void (*start_fn_)(const JsonVariant &),
                    void (*cancel_fn_)())
      : start_fn(start_fn_), cancel_fn(cancel_fn_), name(name_),
        actionRequest(actionRequest_),
        timeRequested("1970-01-01T00:00:00+00:00"), status("created") {
    generateId();
  }

  void generateId() {
    for (uint8_t i = 0; i < 16; ++i) {
      char c = (char)random('0', 'g');

      if (c > '9' && c < 'a') {
        --i;
        continue;
      }

      id += c;
    }
  }

  void serialize(JsonObject obj, String deviceId) {
    JsonObject data = obj.createNestedObject(name);

    JsonObject actionObj = actionRequest->as<JsonObject>();
    data["input"] = actionObj;

    data["status"] = status;
    data["timeRequested"] = timeRequested;

    if (timeCompleted != "") {
      data["timeCompleted"] = timeCompleted;
    }

    data["href"] = "/things/" + deviceId + "/actions/" + name + "/" + id;
  }

  void setStatus(const char *s) {
    status = s;

  }

  void start() {
    setStatus("pending");

    JsonObject actionObj = actionRequest->as<JsonObject>();
    start_fn(actionObj);

    finish();
  }

  void cancel() {
    if (cancel_fn != nullptr) {
      cancel_fn();
    }
  }

  void finish() {
    timeCompleted = "1970-01-01T00:00:00+00:00";
    setStatus("completed");
  }
};

class ThingAction {
private:
  ThingActionObject *(*generator_fn)(DynamicJsonDocument *);

public:
  String id;
  String title;
  String description;
  String type;
  JsonObject *input;
  ThingAction *next = nullptr;

  ThingAction(const char *id_,
              ThingActionObject *(*generator_fn_)(DynamicJsonDocument *))
      : generator_fn(generator_fn_), id(id_) {}

  ThingAction(const char *id_, JsonObject *input_,
              ThingActionObject *(*generator_fn_)(DynamicJsonDocument *))
      : generator_fn(generator_fn_), id(id_), input(input_) {}

  ThingAction(const char *id_, const char *title_, const char *description_,
              const char *type_, JsonObject *input_,
              ThingActionObject *(*generator_fn_)(DynamicJsonDocument *))
      : generator_fn(generator_fn_), id(id_), title(title_),
        description(description_), type(type_), input(input_) {}

  ThingActionObject *create(DynamicJsonDocument *actionRequest) {
    return generator_fn(actionRequest);
  }

  void serialize(JsonObject obj, String deviceId) {
    if (title != "") {
      obj["title"] = title;
    }

    if (description != "") {
      obj["description"] = description;
    }

    if (type != "") {
      obj["@type"] = type;
    }

    if (input != nullptr) {
      JsonObject inputObj = obj.createNestedObject("input");
      for (JsonPair kv : *input) {
        inputObj[kv.key()] = kv.value();
      }
    }

    // 2.11 Action object: A links array (An array of Link objects linking
    // to one or more representations of an Action resource, each with an
    // implied default rel=action.)
    JsonArray inline_links = obj.createNestedArray("forms");
    JsonObject inline_links_prop = inline_links.createNestedObject();
    inline_links_prop["href"] = "/things/" + deviceId + "/actions/" + id;
  }
};

class ThingItem {
public:
  String id;
  String description;
  ThingDataType type;
  String atType;
  ThingItem *next = nullptr;

  bool readOnly = false;
  String unit = "";
  String title = "";
  double minimum = 0;
  double maximum = -1;
  double multipleOf = -1;

  ThingItem(const char *id_, const char *description_, ThingDataType type_,
            const char *atType_)
      : id(id_), description(description_), type(type_), atType(atType_) {}

  void setValue(ThingDataValue newValue) {
    this->value = newValue;
    this->hasChanged = true;
  }

  void setValue(const char *s) {
    *(this->getValue().string) = s;
    this->hasChanged = true;
  }

  /**
   * Returns the property value if it has been changed via {@link setValue}
   * since the last call or returns a nullptr.
   */
  ThingDataValue *changedValueOrNull() {
    ThingDataValue *v = this->hasChanged ? &this->value : nullptr;
    this->hasChanged = false;
    return v;
  }

  ThingDataValue getValue() { return this->value; }

  void serialize(JsonObject obj, String deviceId, String resourceType) {
    switch (type) {
    case NO_STATE:
      break;
    case BOOLEAN:
      obj["type"] = "boolean";
      break;
    case NUMBER:
      obj["type"] = "number";
      break;
    case INTEGER:
      obj["type"] = "integer";
      break;
    case STRING:
      obj["type"] = "string";
      break;
    }

    if (readOnly) {
      obj["readOnly"] = true;
    }

    if (unit != "") {
      obj["unit"] = unit;
    }

    if (title != "") {
      obj["title"] = title;
    }

    if (description != "") {
      obj["description"] = description;
    }

    if (minimum < maximum) {
      obj["minimum"] = minimum;
    }

    if (maximum > minimum) {
      obj["maximum"] = maximum;
    }

    if (multipleOf > 0) {
      obj["multipleOf"] = multipleOf;
    }

    if (atType != nullptr) {
      obj["@type"] = atType;
    }

    // 2.9 Property object: A links array (An array of Link objects linking
    // to one or more representations of a Property resource, each with an
    // implied default rel=property.)
    JsonArray inline_links = obj.createNestedArray("forms");
    JsonObject inline_links_prop = inline_links.createNestedObject();
    inline_links_prop["href"] =
        "/things/" + deviceId + "/" + resourceType + "/" + id;
  }

  void serializeValue(JsonObject prop) {
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
  ThingDataValue value = {false};
  bool hasChanged = false;
};

class ThingProperty : public ThingItem {
private:
  void (*callback)(ThingPropertyValue);

public:
  const char **propertyEnum = nullptr;

  ThingProperty(const char *id_, const char *description_, ThingDataType type_,
                const char *atType_,
                void (*callback_)(ThingPropertyValue) = nullptr)
      : ThingItem(id_, description_, type_, atType_), callback(callback_) {}

  void serialize(JsonObject obj, String deviceId, String resourceType) {
    ThingItem::serialize(obj, deviceId, resourceType);

    const char **enumVal = propertyEnum;
    bool hasEnum = propertyEnum != nullptr && *propertyEnum != nullptr;

    if (hasEnum) {
      enumVal = propertyEnum;
      JsonArray propEnum = obj.createNestedArray("enum");
      while (propertyEnum != nullptr && *enumVal != nullptr) {
        propEnum.add(*enumVal);
        enumVal++;
      }
    }
  }

  void changed(ThingPropertyValue newValue) {
    if (callback != nullptr) {
      callback(newValue);
    }
  }
};
using ThingEvent = ThingItem;

class ThingEventObject {
public:
  String name;
  ThingDataType type;
  ThingDataValue value = {false};
  String timestamp;
  ThingEventObject *next = nullptr;

  ThingEventObject(const char *name_, ThingDataType type_,
                   ThingDataValue value_)
      : name(name_), type(type_), value(value_),
        timestamp("1970-01-01T00:00:00+00:00") {}

  ThingEventObject(const char *name_, ThingDataType type_,
                   ThingDataValue value_, String timestamp_)
      : name(name_), type(type_), value(value_), timestamp(timestamp_) {}

  ThingDataValue getValue() { return this->value; }

  void serialize(JsonObject obj) {
    JsonObject data = obj.createNestedObject(name);
    switch (this->type) {
    case NO_STATE:
      break;
    case BOOLEAN:
      data["data"] = this->getValue().boolean;
      break;
    case NUMBER:
      data["data"] = this->getValue().number;
      break;
    case INTEGER:
      data["data"] = this->getValue().integer;
      break;
    case STRING:
      data["data"] = *this->getValue().string;
      break;
    }
  }
};

class ThingDevice {
public:
  String id;
  String title;
  String description;
  const char **type;
  ThingDevice *next = nullptr;
  ThingProperty *firstProperty = nullptr;
  ThingAction *firstAction = nullptr;
  ThingActionObject *actionQueue = nullptr;
  ThingEvent *firstEvent = nullptr;
  ThingEventObject *eventQueue = nullptr;

  ThingDevice(const char *_id, const char *_title, const char **_type)
      : id(_id), title(_title), type(_type) {}

  ~ThingDevice() {
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

  ThingAction *findAction(const char *id) {
    ThingAction *a = this->firstAction;
    while (a) {
      if (!strcmp(a->id.c_str(), id))
        return a;
      a = (ThingAction *)a->next;
    }
    return nullptr;
  }

  ThingActionObject *findActionObject(const char *id) {
    ThingActionObject *a = this->actionQueue;
    while (a) {
      if (!strcmp(a->id.c_str(), id))
        return a;
      a = a->next;
    }
    return nullptr;
  }

  void addAction(ThingAction *action) {
    action->next = firstAction;
    firstAction = action;
  }

  ThingEvent *findEvent(const char *id) {
    ThingEvent *e = this->firstEvent;
    while (e) {
      if (!strcmp(e->id.c_str(), id))
        return e;
      e = (ThingEvent *)e->next;
    }
    return nullptr;
  }

  void addEvent(ThingEvent *event) {
    event->next = firstEvent;
    firstEvent = event;
  }

  void setProperty(const char *name, const JsonVariant &newValue) {
    ThingProperty *property = findProperty(name);

    if (property == nullptr) {
      return;
    }

    switch (property->type) {
    case NO_STATE: {
      break;
    }
    case BOOLEAN: {
      ThingDataValue value;
      value.boolean = newValue.as<bool>();
      property->setValue(value);
      property->changed(value);
      break;
    }
    case NUMBER: {
      ThingDataValue value;
      value.number = newValue.as<double>();
      property->setValue(value);
      property->changed(value);
      break;
    }
    case INTEGER: {
      ThingDataValue value;
      value.integer = newValue.as<signed long long>();
      property->setValue(value);
      property->changed(value);
      break;
    }
    case STRING:
      property->setValue(newValue.as<const char *>());
      property->changed(property->getValue());
      break;
    }
  }

  ThingActionObject *requestAction(DynamicJsonDocument *actionRequest) {
    JsonObject actionObj = actionRequest->as<JsonObject>();

    // There should only be one key/value pair
    JsonObject::iterator kv = actionObj.begin();
    if (kv == actionObj.end()) {
      return nullptr;
    }

    ThingAction *action = findAction(kv->key().c_str());
    if (action == nullptr) {
      return nullptr;
    }

    ThingActionObject *obj = action->create(actionRequest);
    if (obj == nullptr) {
      return nullptr;
    }

    queueActionObject(obj);
    return obj;
  }

  void removeAction(String id) {
    ThingActionObject *curr = actionQueue;
    ThingActionObject *prev = nullptr;
    while (curr != nullptr) {
      if (curr->id == id) {
        if (prev == nullptr) {
          actionQueue = curr->next;
        } else {
          prev->next = curr->next;
        }

        curr->cancel();
        delete curr->actionRequest;
        delete curr;
        return;
      }

      prev = curr;
      curr = curr->next;
    }
  }

  void queueActionObject(ThingActionObject *obj) {
    obj->next = actionQueue;
    actionQueue = obj;
  }

  void queueEventObject(ThingEventObject *obj) {
    obj->next = eventQueue;
    eventQueue = obj;
  }

  void serialize(JsonObject descr, String ip, uint16_t port) {
    descr["id"] = "uri:" + this->id;
    descr["title"] = this->title;
    JsonArray context = descr.createNestedArray("@context");
    context.add("https://www.w3.org/2019/wot/td/v1");

    if (this->description != "") {
      descr["description"] = this->description;
    }
    if (port != 80) {
      char buffer[33];
      itoa(port, buffer, 10);
      descr["base"] = "http://" + ip + ":" + buffer + "/";
    } else {
      descr["base"] = "http://" + ip + "/";
    }

    JsonObject securityDefinitions =
        descr.createNestedObject("securityDefinitions");
    JsonObject nosecSc = securityDefinitions.createNestedObject("nosec_sc");
    nosecSc["scheme"] = "nosec";
    JsonArray securityJson = descr.createNestedArray("security");
    securityJson.add("nosec_sc");

    JsonArray typeJson = descr.createNestedArray("@type");
    const char **type = this->type;
    while ((*type) != nullptr) {
      typeJson.add(*type);
      type++;
    }

    ThingProperty *property = this->firstProperty;
    if (property != nullptr) {
      JsonArray forms = descr.createNestedArray("forms");
      JsonObject forms_prop = forms.createNestedObject();
      forms_prop["rel"] = "properties";
      JsonArray context = forms_prop.createNestedArray("op");
      context.add("readallproperties");
      context.add("writeallproperties");
      forms_prop["href"] = "/things/" + this->id + "/properties";

      JsonObject properties = descr.createNestedObject("properties");
      while (property != nullptr) {
        JsonObject obj = properties.createNestedObject(property->id);
        property->serialize(obj, id, "properties");
        property = (ThingProperty *)property->next;
      }
    }

    ThingAction *action = this->firstAction;
    if (action != nullptr) {
      JsonObject actions = descr.createNestedObject("actions");
      while (action != nullptr) {
        JsonObject obj = actions.createNestedObject(action->id);
        action->serialize(obj, id);
        action = action->next;
      }
    }

    ThingEvent *event = this->firstEvent;
    if (event != nullptr) {
      JsonObject events = descr.createNestedObject("events");
      while (event != nullptr) {
        JsonObject obj = events.createNestedObject(event->id);
        event->serialize(obj, id, "events");
        event = (ThingEvent *)event->next;
      }
    }
  }

  void serializeActionQueue(JsonArray array) {
    ThingActionObject *curr = actionQueue;
    while (curr != nullptr) {
      JsonObject action = array.createNestedObject();
      curr->serialize(action, id);
      curr = curr->next;
    }
  }

  void serializeActionQueue(JsonArray array, String name) {
    ThingActionObject *curr = actionQueue;
    while (curr != nullptr) {
      if (curr->name == name) {
        JsonObject action = array.createNestedObject();
        curr->serialize(action, id);
      }
      curr = curr->next;
    }
  }

  void serializeEventQueue(JsonArray array) {
    ThingEventObject *curr = eventQueue;
    while (curr != nullptr) {
      JsonObject event = array.createNestedObject();
      curr->serialize(event);
      curr = curr->next;
    }
  }

  void serializeEventQueue(JsonArray array, String name) {
    ThingEventObject *curr = eventQueue;
    while (curr != nullptr) {
      if (curr->name == name) {
        JsonObject event = array.createNestedObject();
        curr->serialize(event);
      }
      curr = curr->next;
    }
  }
};
