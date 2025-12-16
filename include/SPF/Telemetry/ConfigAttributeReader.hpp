#pragma once

#include <optional>
#include <string>
#include <vector>

#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Telemetry/Sdk.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Telemetry {
/**
 * @class ConfigAttributeReader
 * @brief A helper class to safely read attributes from the null-terminated
 *        array provided by SCS_TELEMETRY_EVENT_configuration.
 *
 * This class encapsulates the unsafe C-style iteration and string comparisons,
 * providing a clean, type-safe interface for accessing configuration values.
 */
class ConfigAttributeReader {
 public:
  /**
   * @brief Constructs the reader with the attribute array from the SDK.
   * @param attributes A pointer to the first element of the scs_named_value_t array.
   */
  ConfigAttributeReader(const scs_named_value_t* attributes);

  // --- Getters for single values ---

  std::optional<bool> GetBool(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<int32_t> GetS32(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<uint32_t> GetU32(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<int64_t> GetS64(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<uint64_t> GetU64(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<float> GetFloat(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<std::string> GetString(const char* name, uint32_t index = SCS_U32_NIL) const;
  std::optional<scs_value_fvector_t> GetFVector(const char* name, uint32_t index = SCS_U32_NIL) const;

  // --- Getters for indexed/array values ---

  std::vector<float> GetFloatArray(const char* name, uint32_t count) const;
  std::vector<bool> GetBoolArray(const char* name, uint32_t count) const;
  std::vector<scs_value_fvector_t> GetFVectorArray(const char* name, uint32_t count) const;

 private:
  /**
   * @brief Finds an attribute by its name and index.
   * @param name The name of the attribute (e.g., "truck.brand_id").
   * @param index The index for array attributes, or SCS_U32_NIL for single ones.
   * @return A pointer to the found attribute, or nullptr if not found.
   */
  const scs_named_value_t* FindAttribute(const char* name, uint32_t index) const;

  const scs_named_value_t* m_attributes;
};

}  // namespace Telemetry
SPF_NS_END