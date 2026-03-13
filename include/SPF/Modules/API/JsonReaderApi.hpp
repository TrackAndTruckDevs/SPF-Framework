#pragma once

#include "SPF/SPF_API/SPF_JsonReader_API.h"
#include "SPF/Namespace.hpp"

#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules::API {
class JsonReaderApi {
 public:
  static void FillJsonReaderApi(SPF_JsonReader_API* api);

 private:
  static SPF_JsonType Json_GetType(const SPF_JsonValue_Handle* h);
  static bool Json_GetBool(const SPF_JsonValue_Handle* h, bool default_value);
  static int64_t Json_GetInt(const SPF_JsonValue_Handle* h, int64_t default_value);
  static int32_t Json_GetInt32(const SPF_JsonValue_Handle* h, int32_t default_value);
  static uint64_t Json_GetUint(const SPF_JsonValue_Handle* h, uint64_t default_value);
  static double Json_GetFloat(const SPF_JsonValue_Handle* h, double default_value);
  static int Json_GetString(const SPF_JsonValue_Handle* h, char* out_buffer, int buffer_size);
  static bool Json_HasMember(const SPF_JsonValue_Handle* h, const char* memberName);
  static SPF_JsonValue_Handle* Json_GetMember(const SPF_JsonValue_Handle* h, const char* memberName);
  static int Json_GetArraySize(const SPF_JsonValue_Handle* h);
  static SPF_JsonValue_Handle* Json_GetArrayItem(const SPF_JsonValue_Handle* h, int index);
  static int Json_GetObjectSize(const SPF_JsonValue_Handle* h);
  static int Json_GetMemberName(const SPF_JsonValue_Handle* h, int index, char* out_buffer, int buffer_size);
  static SPF_JsonValue_Handle* Json_GetMemberValueByIndex(const SPF_JsonValue_Handle* h, int index);
};
}  // namespace Modules::API
SPF_NS_END
