#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Config/ManifestData.hpp"        // For the C++ ManifestData structure
#include "SPF/SPF_API/SPF_Manifest_API.h"  // For the C-compatible SPF_ManifestData_C structure

SPF_NS_BEGIN
namespace Modules::API {

/**
 * @brief Provides functionality to convert C-compatible manifest structures
 *        into C++ ManifestData objects.
 * This API acts as a centralized utility for handling manifest data conversion,
 * ensuring consistency and isolating the conversion logic from other components
 * like PluginManager.
 */
class ManifestApi {
 public:
  /**
   * @brief Converts a C-compatible SPF_ManifestData_C structure into a C++ ManifestData object.
   * This function is designed to be robust, handling potential data conversion errors
   * by logging them and returning a partially filled ManifestData object in case of failure.
   *
   * @param cManifest A pointer to the C-compatible SPF_ManifestData_C structure provided by a plugin.
   * @param pluginName The name of the plugin providing the manifest (for logging purposes).
   * @return A C++ ManifestData object representing the converted manifest.
   */
  static SPF::Config::ManifestData ConvertCManifestToCppManifest(const SPF_ManifestData_C& cManifest, const std::string& pluginName);
};

}  // namespace Modules::API

SPF_NS_END