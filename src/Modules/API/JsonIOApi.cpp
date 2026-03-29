#include "SPF/Modules/API/JsonIOApi.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <cstring>

SPF_NS_BEGIN
namespace Modules::API {

SPF_JsonValue_Handle* JsonIOApi::Json_ParseString(const char* jsonString) {
    if (!jsonString) return nullptr;
    try {
        auto* json = new nlohmann::ordered_json(nlohmann::ordered_json::parse(jsonString));
        return reinterpret_cast<SPF_JsonValue_Handle*>(json);
    } catch (...) {
        return nullptr;
    }
}

SPF_JsonValue_Handle* JsonIOApi::Json_LoadFromFile(const char* filePath) {
    if (!filePath) return nullptr;
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) return nullptr;
        
        auto* json = new nlohmann::ordered_json();
        file >> *json;
        return reinterpret_cast<SPF_JsonValue_Handle*>(json);
    } catch (...) {
        return nullptr;
    }
}

int JsonIOApi::Json_ToString(const SPF_JsonValue_Handle* h, bool prettyPrint, char* out_buffer, int buffer_size) {
    if (!h || !out_buffer || buffer_size <= 0) return 0;
    try {
        auto* json = reinterpret_cast<const nlohmann::ordered_json*>(h);
        std::string s = json->dump(prettyPrint ? 4 : -1);
        
        if (s.length() < static_cast<size_t>(buffer_size)) {
            strcpy_s(out_buffer, buffer_size, s.c_str());
            return static_cast<int>(s.length());
        } else {
            *out_buffer = '\0';
            return static_cast<int>(s.length()) + 1;
        }
    } catch (...) {
        *out_buffer = '\0';
        return 0;
    }
}

bool JsonIOApi::Json_SaveToFile(const SPF_JsonValue_Handle* h, const char* filePath, bool prettyPrint) {
    if (!h || !filePath) return false;
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;
        
        auto* json = reinterpret_cast<const nlohmann::ordered_json*>(h);
        file << json->dump(prettyPrint ? 4 : -1);
        return true;
    } catch (...) {
        return false;
    }
}

bool JsonIOApi::Json_IsValid(const char* jsonString) {
    if (!jsonString) return false;
    return nlohmann::ordered_json::accept(jsonString);
}

void JsonIOApi::FillJsonIOApi(SPF_JsonIO_API* api) {
    if (!api) return;
    api->Json_ParseString = &JsonIOApi::Json_ParseString;
    api->Json_LoadFromFile = &JsonIOApi::Json_LoadFromFile;
    api->Json_ToString = &JsonIOApi::Json_ToString;
    api->Json_SaveToFile = &JsonIOApi::Json_SaveToFile;
    api->Json_IsValid = &JsonIOApi::Json_IsValid;
}

} // namespace Modules::API
SPF_NS_END
