#pragma once

#include "SPF/Namespace.hpp"

#include <string>


SPF_NS_BEGIN

namespace Utils {

class SystemUtils {
 public:
  /**
   * @brief Retrieves the current system locale name (e.g., "uk-UA" or "en-US").
   *
   * @return std::string The locale name, or "en-US" as a fallback.
   */
  static std::string GetSystemLocaleName();

  /**
   * @brief Retrieves the OS version and build information (e.g., "Windows 11 (Build 22631)").
   *
   * @return std::string The formatted OS version string.
   */
  static std::string GetOSVersionString();

  /**
   * @brief Retrieves the system architecture.
   *
   * @return std::string The architecture string (e.g., "x64").
   */
  static std::string GetSystemArchitecture();
};

}  // namespace Utils

SPF_NS_END
