#include "SPF/Modules/HandleManager.hpp"

#include <list>
#include <map>

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Modules {
// --- PIMPL Implementation ---
class HandleManager::HandleManagerImpl {
 public:
  Handles::IHandle* RegisterHandle(const std::string& pluginName, std::unique_ptr<Handles::IHandle> handle) {
    // std::list provides pointer stability on insertion.
    auto& handleList = m_handles[pluginName];
    handleList.push_back(std::move(handle));
    // The returned raw pointer is stable.
    return handleList.back().get();
  }

  void ReleaseHandlesFor(const std::string& pluginName) {
    auto it = m_handles.find(pluginName);
    if (it != m_handles.end()) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("HandleManager");
      logger->Info("Releasing {} handle(s) for unloaded plugin: '{}'", it->second.size(), pluginName);
      m_handles.erase(it);
    }
  }

 private:
  // Maps a plugin name to a list of handles it owns.
  // Using std::list to ensure pointer stability.
  std::map<std::string, std::list<std::unique_ptr<Handles::IHandle>>> m_handles;
};

// --- Public HandleManager Methods ---

HandleManager::HandleManager() : m_pimpl(std::make_unique<HandleManagerImpl>()) {}

HandleManager::~HandleManager() = default;

Handles::IHandle* HandleManager::RegisterHandle(const std::string& pluginName, std::unique_ptr<Handles::IHandle> handle) {
  return m_pimpl->RegisterHandle(pluginName, std::move(handle));
}

void HandleManager::ReleaseHandlesFor(const std::string& pluginName) { m_pimpl->ReleaseHandlesFor(pluginName); }
}  // namespace Modules
SPF_NS_END
