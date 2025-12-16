#pragma once

#include "SPF/Namespace.hpp"
#include <string>

SPF_NS_BEGIN

namespace Config {
class IConfigService;
}  // namespace Config

namespace Hooks {
/**
 * @class IHook
 * @brief An interface for a manageable, configurable feature hook.
 */
class IHook {
 public:
  virtual ~IHook() = default;

  virtual const std::string& GetName() const = 0;
  virtual const std::string& GetDisplayName() const = 0;

  /**
   * @brief Gets the name of the component that owns this hook (e.g., "framework" or "MyPlugin").
   * @return The owner component name.
   */
  virtual const std::string& GetOwnerName() const = 0;
  virtual bool IsEnabled() const = 0;
  virtual void SetEnabled(bool enabled) = 0;
  virtual const std::string& GetSignature() const = 0;
  virtual bool IsInstalled() const = 0;
  virtual bool Install() = 0;
  virtual void Uninstall() = 0;
  virtual void Remove() = 0;
};
}  // namespace Hooks
SPF_NS_END
