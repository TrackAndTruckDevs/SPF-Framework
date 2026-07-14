#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_JsonReader_API.h"
#include "SPF/SPF_API/SPF_JsonWriter_API.h"

#include <cstdint>


SPF_NS_BEGIN
namespace Modules::API {
class JsonWriterApi {
 public:
  static void FillJsonWriterApi(SPF_JsonWriter_API* api);

 private:
  static SPF_JsonValue_Handle* Json_CreateObject();
  static SPF_JsonValue_Handle* Json_CreateArray();
  static void Json_DestroyHandle(SPF_JsonValue_Handle* h);

  // --- Object Modification ---
  static void Json_SetInt(SPF_JsonValue_Handle* h, const char* key, int64_t value);
  static void Json_SetDouble(SPF_JsonValue_Handle* h, const char* key, double value);
  static void Json_SetBool(SPF_JsonValue_Handle* h, const char* key, bool value);
  static void Json_SetString(SPF_JsonValue_Handle* h, const char* key, const char* value);
  static void Json_SetNode(SPF_JsonValue_Handle* h, const char* key, SPF_JsonValue_Handle* nodeHandle);

  // --- Array Modification ---
  static void Json_ArrayAppendInt(SPF_JsonValue_Handle* h, int64_t value);
  static void Json_ArrayAppendDouble(SPF_JsonValue_Handle* h, double value);
  static void Json_ArrayAppendBool(SPF_JsonValue_Handle* h, bool value);
  static void Json_ArrayAppendString(SPF_JsonValue_Handle* h, const char* value);
  static void Json_ArrayAppendNode(SPF_JsonValue_Handle* h, SPF_JsonValue_Handle* nodeHandle);

  // --- Utility ---
  static void Json_RemoveMember(SPF_JsonValue_Handle* h, const char* key);
  static void Json_RemoveArrayItem(SPF_JsonValue_Handle* h, int index);
  static void Json_Clear(SPF_JsonValue_Handle* h);
};
}  // namespace Modules::API
SPF_NS_END
