#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Config/ManifestData.hpp"        // For the C++ ManifestData structure
#include "SPF/SPF_API/SPF_Manifest_API.h"   // For SPF_GetManifestAPI_Func and Builder types

#include <string>

SPF_NS_BEGIN
namespace Modules::API {

/**
 * @brief Handles the construction of plugin manifests using the C-API Builder pattern.
 */
class ManifestApi {
 public:
  /**
   * @brief Invokes the plugin's builder function to populate a C++ ManifestData object.
   * 
   * @param pGetManifestFunc Pointer to the plugin's exported `SPF_GetManifestAPI` function.
   * @param pluginName The name of the plugin (used for initial fallback and logging).
   * @return A fully populated C++ ManifestData object.
   */
  static SPF::Config::ManifestData BuildManifest(SPF_GetManifestAPI_Func pGetManifestFunc, const std::string& pluginName);
};

}  // namespace Modules::API

SPF_NS_END
