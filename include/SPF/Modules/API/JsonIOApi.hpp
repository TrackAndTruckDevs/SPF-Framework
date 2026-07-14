#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_JsonIO_API.h"
#include "SPF/SPF_API/SPF_JsonReader_API.h"


SPF_NS_BEGIN
namespace Modules::API {
class JsonIOApi {
 public:
  static void FillJsonIOApi(SPF_JsonIO_API* api);

 private:
  static SPF_JsonValue_Handle* Json_ParseString(const char* jsonString);
  static SPF_JsonValue_Handle* Json_LoadFromFile(const char* filePath);
  static int Json_ToString(const SPF_JsonValue_Handle* h, bool prettyPrint, char* out_buffer, int buffer_size);
  static bool Json_SaveToFile(const SPF_JsonValue_Handle* h, const char* filePath, bool prettyPrint);
  static bool Json_IsValid(const char* jsonString);
};
}  // namespace Modules::API
SPF_NS_END
