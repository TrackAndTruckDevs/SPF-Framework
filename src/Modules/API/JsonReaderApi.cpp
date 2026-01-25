#include "SPF/Modules/API/JsonReaderApi.hpp"
#include <nlohmann/json.hpp>
#include <cstring> // For strcpy_s
#include <algorithm> // For std::min

SPF_NS_BEGIN
namespace Modules::API {

SPF_JsonType JsonReaderApi::Json_GetType(const SPF_JsonValue_Handle* h) {
  if (!h) return SPF_JSON_TYPE_UNKNOWN;
  const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
  if (json_value->is_null()) return SPF_JSON_TYPE_NULL;
  if (json_value->is_object()) return SPF_JSON_TYPE_OBJECT;
  if (json_value->is_array()) return SPF_JSON_TYPE_ARRAY;
  if (json_value->is_string()) return SPF_JSON_TYPE_STRING;
  if (json_value->is_boolean()) return SPF_JSON_TYPE_BOOLEAN;
  if (json_value->is_number_unsigned()) return SPF_JSON_TYPE_NUMBER_UNSIGNED;
  if (json_value->is_number_integer()) return SPF_JSON_TYPE_NUMBER_INTEGER;
  if (json_value->is_number_float()) return SPF_JSON_TYPE_NUMBER_FLOAT;
  return SPF_JSON_TYPE_UNKNOWN;
}

bool JsonReaderApi::Json_GetBool(const SPF_JsonValue_Handle* h, bool default_value) {
  if (!h) return default_value;
  const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
  if (!json_value->is_boolean()) return default_value;
  return json_value->get<bool>();
}

int64_t JsonReaderApi::Json_GetInt(const SPF_JsonValue_Handle* h, int64_t default_value) {
  if (!h) return default_value;
  const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
  if (!json_value->is_number_integer()) return default_value;
  return json_value->get<int64_t>();
}

int32_t JsonReaderApi::Json_GetInt32(const SPF_JsonValue_Handle* h, int32_t default_value) {
    if (!h) return default_value;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_integer()) return default_value;
    return static_cast<int32_t>(json_value->get<int64_t>());
}

uint64_t JsonReaderApi::Json_GetUint(const SPF_JsonValue_Handle* h, uint64_t default_value) {
    if (!h) return default_value;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_unsigned()) return default_value;
    return json_value->get<uint64_t>();
}

double JsonReaderApi::Json_GetFloat(const SPF_JsonValue_Handle* h, double default_value) {
  if (!h) return default_value;
  const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
  if (!json_value->is_number_float()) return default_value;
  return json_value->get<double>();
}

int JsonReaderApi::Json_GetString(const SPF_JsonValue_Handle* h, char* out_buffer, int buffer_size) {
  if (!h || !out_buffer || buffer_size <= 0) return 0;
  const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
  if (!json_value->is_string()) {
    *out_buffer = '\0';
    return 0;
  }

  std::string value_str = json_value->get<std::string>();
  if (value_str.length() < buffer_size) {
    strcpy_s(out_buffer, buffer_size, value_str.c_str());
    return value_str.length();
  } else {
    *out_buffer = '\0';
    return value_str.length() + 1;  // Return required size
  }
}

bool JsonReaderApi::Json_HasMember(const SPF_JsonValue_Handle* h, const char* memberName) {
    if (!h || !memberName) return false;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object()) return false;
    return json_value->contains(memberName);
}

SPF_JsonValue_Handle* JsonReaderApi::Json_GetMember(const SPF_JsonValue_Handle* h, const char* memberName) {
    if (!h || !memberName) return nullptr;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object() || !json_value->contains(memberName)) return nullptr;
    
    const nlohmann::ordered_json& member = json_value->at(memberName);
    return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(&member));
}

int JsonReaderApi::Json_GetArraySize(const SPF_JsonValue_Handle* h) {
    if (!h) return 0;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_array()) return 0;
    return static_cast<int>(json_value->size());
}

SPF_JsonValue_Handle* JsonReaderApi::Json_GetArrayItem(const SPF_JsonValue_Handle* h, int index) {
    if (!h) return nullptr;
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_array() || index < 0 || static_cast<size_t>(index) >= json_value->size()) {
        return nullptr;
    }

    const nlohmann::ordered_json& item = json_value->at(index);
    return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(&item));
}

void JsonReaderApi::FillJsonReaderApi(SPF_JsonReader_API* api) {
    if (!api) return;

    api->Json_GetType = &JsonReaderApi::Json_GetType;
    api->Json_GetBool = &JsonReaderApi::Json_GetBool;
    api->Json_GetInt = &JsonReaderApi::Json_GetInt;
    api->Json_GetInt32 = &JsonReaderApi::Json_GetInt32;
    api->Json_GetUint = &JsonReaderApi::Json_GetUint;
    api->Json_GetFloat = &JsonReaderApi::Json_GetFloat;
    api->Json_GetString = &JsonReaderApi::Json_GetString;
    api->Json_HasMember = &JsonReaderApi::Json_HasMember;
    api->Json_GetMember = &JsonReaderApi::Json_GetMember;
    api->Json_GetArraySize = &JsonReaderApi::Json_GetArraySize;
    api->Json_GetArrayItem = &JsonReaderApi::Json_GetArrayItem;
}

} // namespace Modules::API
SPF_NS_END
