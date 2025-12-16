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
  static SPF_JsonType Json_GetType(const SPF_JsonValue_Handle* handle);
  static bool Json_GetBool(const SPF_JsonValue_Handle* handle, bool default_value);
  static int64_t Json_GetInt(const SPF_JsonValue_Handle* handle, int64_t default_value);
  static int32_t Json_GetInt32(const SPF_JsonValue_Handle* handle, int32_t default_value);
  static uint64_t Json_GetUint(const SPF_JsonValue_Handle* handle, uint64_t default_value);
  static double Json_GetFloat(const SPF_JsonValue_Handle* handle, double default_value);
  static int Json_GetString(const SPF_JsonValue_Handle* handle, char* out_buffer, int buffer_size);
};
}  // namespace Modules::API
SPF_NS_END
