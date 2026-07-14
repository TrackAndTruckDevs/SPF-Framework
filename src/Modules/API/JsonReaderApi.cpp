#include "SPF/Modules/API/JsonReaderApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/SPF_API/SPF_JsonReader_API.h"

#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <cstring>  // For strcpy_s
#include <exception>
#include <iterator>
#include <string>

// IWYU insists on a direct provider for _s functions.
// MinGW: pull in MSVC-compat decl; MSVC gets them from <cstdio> natively.
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <sec_api/string_s.h>
#endif

SPF_NS_BEGIN
namespace Modules::API {

using namespace Logging;

SPF_JsonType JsonReaderApi::Json_GetType(const SPF_JsonValue_Handle* h) {
  if (!h) return SPF_JSON_TYPE_UNKNOWN;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (json_value->is_null()) return SPF_JSON_TYPE_NULL;
    if (json_value->is_object()) return SPF_JSON_TYPE_OBJECT;
    if (json_value->is_array()) return SPF_JSON_TYPE_ARRAY;
    if (json_value->is_string()) return SPF_JSON_TYPE_STRING;
    if (json_value->is_boolean()) return SPF_JSON_TYPE_BOOLEAN;
    if (json_value->is_number_unsigned()) return SPF_JSON_TYPE_NUMBER_UNSIGNED;
    if (json_value->is_number_integer()) return SPF_JSON_TYPE_NUMBER_INTEGER;
    if (json_value->is_number_float()) return SPF_JSON_TYPE_NUMBER_FLOAT;
  } catch (...) {
  }
  return SPF_JSON_TYPE_UNKNOWN;
}

bool JsonReaderApi::Json_GetBool(const SPF_JsonValue_Handle* h, bool default_value) {
  if (!h) return default_value;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_boolean()) return default_value;
    return json_value->get<bool>();
  } catch (...) {
    return default_value;
  }
}

int64_t JsonReaderApi::Json_GetInt(const SPF_JsonValue_Handle* h, int64_t default_value) {
  if (!h) return default_value;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_integer()) return default_value;
    return json_value->get<int64_t>();
  } catch (...) {
    return default_value;
  }
}

int32_t JsonReaderApi::Json_GetInt32(const SPF_JsonValue_Handle* h, int32_t default_value) {
  if (!h) return default_value;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_integer()) return default_value;
    return static_cast<int32_t>(json_value->get<int64_t>());
  } catch (...) {
    return default_value;
  }
}

uint64_t JsonReaderApi::Json_GetUint(const SPF_JsonValue_Handle* h, uint64_t default_value) {
  if (!h) return default_value;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_unsigned()) return default_value;
    return json_value->get<uint64_t>();
  } catch (...) {
    return default_value;
  }
}

double JsonReaderApi::Json_GetFloat(const SPF_JsonValue_Handle* h, double default_value) {
  if (!h) return default_value;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_number_float()) return default_value;
    return json_value->get<double>();
  } catch (...) {
    return default_value;
  }
}

int JsonReaderApi::Json_GetString(const SPF_JsonValue_Handle* h, char* out_buffer, int buffer_size) {
  if (!h || !out_buffer || buffer_size <= 0) return 0;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_string()) {
      *out_buffer = '\0';
      return 0;
    }

    std::string value_str = json_value->get<std::string>();
    if (value_str.length() < static_cast<size_t>(buffer_size)) {
      strcpy_s(out_buffer, buffer_size, value_str.c_str());
      return static_cast<int>(value_str.length());
    } else {
      *out_buffer = '\0';
      return static_cast<int>(value_str.length()) + 1;
    }
  } catch (...) {
    *out_buffer = '\0';
    return 0;
  }
}

bool JsonReaderApi::Json_HasMember(const SPF_JsonValue_Handle* h, const char* memberName) {
  if (!h || !memberName) return false;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object()) return false;
    return json_value->contains(memberName);
  } catch (...) {
    return false;
  }
}

SPF_JsonValue_Handle* JsonReaderApi::Json_GetMember(const SPF_JsonValue_Handle* h, const char* memberName) {
  if (!h || !memberName) return nullptr;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object() || !json_value->contains(memberName)) return nullptr;

    const nlohmann::ordered_json& member = json_value->at(memberName);
    return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(&member));
  } catch (...) {
    return nullptr;
  }
}

int JsonReaderApi::Json_GetArraySize(const SPF_JsonValue_Handle* h) {
  if (!h) return 0;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_array()) return 0;
    return static_cast<int>(json_value->size());
  } catch (...) {
    return 0;
  }
}

SPF_JsonValue_Handle* JsonReaderApi::Json_GetArrayItem(const SPF_JsonValue_Handle* h, int index) {
  if (!h) return nullptr;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_array() || index < 0 || static_cast<size_t>(index) >= json_value->size()) {
      return nullptr;
    }

    const nlohmann::ordered_json& item = json_value->at(index);
    return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(&item));
  } catch (...) {
    return nullptr;
  }
}

int JsonReaderApi::Json_GetObjectSize(const SPF_JsonValue_Handle* h) {
  if (!h) return 0;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object()) return 0;
    return static_cast<int>(json_value->size());
  } catch (...) {
    return 0;
  }
}

int JsonReaderApi::Json_GetMemberName(const SPF_JsonValue_Handle* h, int index, char* out_buffer, int buffer_size) {
  if (!h || !out_buffer || buffer_size <= 0) return 0;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object() || index < 0 || static_cast<size_t>(index) >= json_value->size()) {
      *out_buffer = '\0';
      return 0;
    }

    auto it = json_value->begin();
    std::advance(it, index);
    const std::string& key = it.key();

    if (key.length() < static_cast<size_t>(buffer_size)) {
      strcpy_s(out_buffer, buffer_size, key.c_str());
      return static_cast<int>(key.length());
    } else {
      *out_buffer = '\0';
      return static_cast<int>(key.length()) + 1;
    }
  } catch (const std::exception& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("JsonReaderApi");
    if (logger) logger->Error("Json_GetMemberName: Error at index {}. Error: {}", index, e.what());
    *out_buffer = '\0';
    return 0;
  }
}

SPF_JsonValue_Handle* JsonReaderApi::Json_GetMemberValueByIndex(const SPF_JsonValue_Handle* h, int index) {
  if (!h) return nullptr;
  try {
    const auto* json_value = reinterpret_cast<const nlohmann::ordered_json*>(h);
    if (!json_value->is_object() || index < 0 || static_cast<size_t>(index) >= json_value->size()) {
      return nullptr;
    }

    auto it = json_value->begin();
    std::advance(it, index);
    const nlohmann::ordered_json& val = it.value();
    return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(&val));
  } catch (const std::exception& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("JsonReaderApi");
    if (logger) logger->Error("Json_GetMemberValueByIndex: Error at index {}. Error: {}", index, e.what());
    return nullptr;
  }
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
  api->Json_GetObjectSize = &JsonReaderApi::Json_GetObjectSize;
  api->Json_GetMemberName = &JsonReaderApi::Json_GetMemberName;
  api->Json_GetMemberValueByIndex = &JsonReaderApi::Json_GetMemberValueByIndex;
}

}  // namespace Modules::API
SPF_NS_END
