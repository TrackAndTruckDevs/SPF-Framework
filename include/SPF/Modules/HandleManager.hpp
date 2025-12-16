#pragma once

#include <string>
#include <memory>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Handles {
struct IHandle;
}

namespace Modules {
/**
 * @class HandleManager
 * @brief An internal service that manages the lifecycle of API handles.
 *
 * This class is responsible for tracking all handles created by plugins
 * and ensuring they are properly destroyed when a plugin is unloaded,
 * preventing memory leaks.
 *
 * It uses the PIMPL idiom to hide implementation details.
 */
class HandleManager {
 public:
  HandleManager();
  ~HandleManager();

  HandleManager(const HandleManager&) = delete;
  HandleManager& operator=(const HandleManager&) = delete;
  HandleManager(HandleManager&&) = delete;
  HandleManager& operator=(HandleManager&&) = delete;

  /**
   * @brief Registers a new handle and associates it with a plugin.
   * @param pluginName The name of the plugin that owns the handle.
   * @param handle A unique_ptr to the handle to be managed.
   * @return A stable, non-owning pointer to the handle that was just registered.
   */
  Handles::IHandle* RegisterHandle(const std::string& pluginName, std::unique_ptr<Handles::IHandle> handle);

  /**
   * @brief Destroys all handles associated with a specific plugin.
   * @param pluginName The name of the plugin whose handles should be released.
   */
  void ReleaseHandlesFor(const std::string& pluginName);

 private:
  class HandleManagerImpl;
  std::unique_ptr<HandleManagerImpl> m_pimpl;
};
}  // namespace Modules

SPF_NS_END
