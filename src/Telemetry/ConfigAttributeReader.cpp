#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include <cstring>  // For strcmp

SPF_NS_BEGIN
namespace Telemetry {
ConfigAttributeReader::ConfigAttributeReader(const scs_named_value_t* attributes) : m_attributes(attributes) {}

const scs_named_value_t* ConfigAttributeReader::FindAttribute(const char* name, uint32_t index) const {
  if (!m_attributes) return nullptr;

  for (const scs_named_value_t* current = m_attributes; current->name != nullptr; ++current) {
    if (current->index == index && strcmp(current->name, name) == 0) {
      return current;
    }
  }
  return nullptr;
}

std::optional<bool> ConfigAttributeReader::GetBool(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_bool) {
    return attr->value.value_bool.value != 0;
  }
  return std::nullopt;
}

std::optional<int32_t> ConfigAttributeReader::GetS32(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_s32) {
    return attr->value.value_s32.value;
  }
  return std::nullopt;
}

std::optional<uint32_t> ConfigAttributeReader::GetU32(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_u32) {
    return attr->value.value_u32.value;
  }
  return std::nullopt;
}

std::optional<int64_t> ConfigAttributeReader::GetS64(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_s64) {
    return attr->value.value_s64.value;
  }
  return std::nullopt;
}

std::optional<uint64_t> ConfigAttributeReader::GetU64(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_u64) {
    return attr->value.value_u64.value;
  }
  return std::nullopt;
}

std::optional<float> ConfigAttributeReader::GetFloat(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_float) {
    return attr->value.value_float.value;
  }
  return std::nullopt;
}

std::optional<std::string> ConfigAttributeReader::GetString(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_string) {
    return std::string(attr->value.value_string.value);
  }
  return std::nullopt;
}

std::optional<scs_value_fvector_t> ConfigAttributeReader::GetFVector(const char* name, uint32_t index) const {
  const auto* attr = FindAttribute(name, index);
  if (attr && attr->value.type == SCS_VALUE_TYPE_fvector) {
    return attr->value.value_fvector;
  }
  return std::nullopt;
}

std::vector<float> ConfigAttributeReader::GetFloatArray(const char* name, uint32_t count) const {
  std::vector<float> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    if (auto val = GetFloat(name, i)) {
      result.push_back(*val);
    } else {
      // Push a default value or handle the error if an element is missing
      result.push_back(0.0f);
    }
  }
  return result;
}

std::vector<bool> ConfigAttributeReader::GetBoolArray(const char* name, uint32_t count) const {
  std::vector<bool> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    if (auto val = GetBool(name, i)) {
      result.push_back(*val);
    } else {
      result.push_back(false);
    }
  }
  return result;
}

std::vector<scs_value_fvector_t> ConfigAttributeReader::GetFVectorArray(const char* name, uint32_t count) const {
  std::vector<scs_value_fvector_t> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    if (auto val = GetFVector(name, i)) {
      result.push_back(*val);
    } else {
      result.push_back({0.0f, 0.0f, 0.0f});  // Default value
    }
  }
  return result;
}

}  // namespace Telemetry
SPF_NS_END