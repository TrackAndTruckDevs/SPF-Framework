#include "SPF/Hooks/PluginHook.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Modules {
PluginHook::PluginHook(const std::string& pluginName, const std::string& hookName, const std::string& displayName, void* pDetour, void** ppOriginal, const std::string& signature, bool isEnabled)
    : BaseHook(hookName, displayName, signature, pluginName, isEnabled), m_pDetour(pDetour), m_ppOriginal(ppOriginal) {
    // m_ownerName is now initialized in BaseHook constructor
}

PluginHook::~PluginHook() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetOwnerName());
  if (logger) logger->Info("--> [2.1/3] ~PluginHook() for '''{}''' called. Calling Remove()...", GetDisplayName());
  // Automatically clean up the hook when the C++ object is destroyed.
  BaseHook::Remove();
  if (logger) logger->Info("--> [2.2/3] ~PluginHook() for '''{}''' finished.", GetDisplayName());
}

void* PluginHook::GetDetourFunc() {
    return m_pDetour;
}

void** PluginHook::GetOriginalFuncPtr() {
    return m_ppOriginal;
}

}  // namespace Modules
SPF_NS_END
