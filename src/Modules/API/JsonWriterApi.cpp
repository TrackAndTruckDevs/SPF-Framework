#include "SPF/Modules/API/JsonWriterApi.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules::API {

SPF_JsonValue_Handle* JsonWriterApi::Json_CreateObject() {
    return reinterpret_cast<SPF_JsonValue_Handle*>(new nlohmann::ordered_json(nlohmann::ordered_json::value_t::object));
}

SPF_JsonValue_Handle* JsonWriterApi::Json_CreateArray() {
    return reinterpret_cast<SPF_JsonValue_Handle*>(new nlohmann::ordered_json(nlohmann::ordered_json::value_t::array));
}

void JsonWriterApi::Json_DestroyHandle(SPF_JsonValue_Handle* h) {
    if (h) {
        delete reinterpret_cast<nlohmann::ordered_json*>(h);
    }
}

// --- Object Modification ---

void JsonWriterApi::Json_SetInt(SPF_JsonValue_Handle* h, const char* key, int64_t value) {
    if (!h || !key) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_object()) {
            (*json)[key] = value;
        }
    } catch (...) {}
}

void JsonWriterApi::Json_SetDouble(SPF_JsonValue_Handle* h, const char* key, double value) {
    if (!h || !key) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_object()) {
            (*json)[key] = value;
        }
    } catch (...) {}
}

void JsonWriterApi::Json_SetBool(SPF_JsonValue_Handle* h, const char* key, bool value) {
    if (!h || !key) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_object()) {
            (*json)[key] = value;
        }
    } catch (...) {}
}

void JsonWriterApi::Json_SetString(SPF_JsonValue_Handle* h, const char* key, const char* value) {
    if (!h || !key || !value) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_object()) {
            (*json)[key] = value;
        }
    } catch (...) {}
}

void JsonWriterApi::Json_SetNode(SPF_JsonValue_Handle* h, const char* key, SPF_JsonValue_Handle* nodeHandle) {
    if (!h || !key || !nodeHandle) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        auto* node = reinterpret_cast<nlohmann::ordered_json*>(nodeHandle);
        if (json->is_object()) {
            (*json)[key] = *node;
        }
    } catch (...) {}
}

// --- Array Modification ---

void JsonWriterApi::Json_ArrayAppendInt(SPF_JsonValue_Handle* h, int64_t value) {
    if (!h) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_array()) {
            json->push_back(value);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_ArrayAppendDouble(SPF_JsonValue_Handle* h, double value) {
    if (!h) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_array()) {
            json->push_back(value);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_ArrayAppendBool(SPF_JsonValue_Handle* h, bool value) {
    if (!h) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_array()) {
            json->push_back(value);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_ArrayAppendString(SPF_JsonValue_Handle* h, const char* value) {
    if (!h || !value) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_array()) {
            json->push_back(value);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_ArrayAppendNode(SPF_JsonValue_Handle* h, SPF_JsonValue_Handle* nodeHandle) {
    if (!h || !nodeHandle) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        auto* node = reinterpret_cast<nlohmann::ordered_json*>(nodeHandle);
        if (json->is_array()) {
            json->push_back(*node);
        }
    } catch (...) {}
}

// --- Utility ---

void JsonWriterApi::Json_RemoveMember(SPF_JsonValue_Handle* h, const char* key) {
    if (!h || !key) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_object()) {
            json->erase(key);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_RemoveArrayItem(SPF_JsonValue_Handle* h, int index) {
    if (!h || index < 0) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        if (json->is_array() && static_cast<size_t>(index) < json->size()) {
            json->erase(json->begin() + index);
        }
    } catch (...) {}
}

void JsonWriterApi::Json_Clear(SPF_JsonValue_Handle* h) {
    if (!h) return;
    try {
        auto* json = reinterpret_cast<nlohmann::ordered_json*>(h);
        json->clear();
    } catch (...) {}
}

void JsonWriterApi::FillJsonWriterApi(SPF_JsonWriter_API* api) {
    if (!api) return;
    api->Json_CreateObject = &JsonWriterApi::Json_CreateObject;
    api->Json_CreateArray = &JsonWriterApi::Json_CreateArray;
    api->Json_DestroyHandle = &JsonWriterApi::Json_DestroyHandle;
    api->Json_SetInt = &JsonWriterApi::Json_SetInt;
    api->Json_SetDouble = &JsonWriterApi::Json_SetDouble;
    api->Json_SetBool = &JsonWriterApi::Json_SetBool;
    api->Json_SetString = &JsonWriterApi::Json_SetString;
    api->Json_SetNode = &JsonWriterApi::Json_SetNode;
    api->Json_ArrayAppendInt = &JsonWriterApi::Json_ArrayAppendInt;
    api->Json_ArrayAppendDouble = &JsonWriterApi::Json_ArrayAppendDouble;
    api->Json_ArrayAppendBool = &JsonWriterApi::Json_ArrayAppendBool;
    api->Json_ArrayAppendString = &JsonWriterApi::Json_ArrayAppendString;
    api->Json_ArrayAppendNode = &JsonWriterApi::Json_ArrayAppendNode;
    api->Json_RemoveMember = &JsonWriterApi::Json_RemoveMember;
    api->Json_RemoveArrayItem = &JsonWriterApi::Json_RemoveArrayItem;
    api->Json_Clear = &JsonWriterApi::Json_Clear;
}

} // namespace Modules::API
SPF_NS_END
