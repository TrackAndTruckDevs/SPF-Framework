#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Hooks {
struct DInput8Hook {
  static bool Install();
  static void Uninstall();  // Disables the hook for reload
  static void Remove();     // Completely removes the hook for shutdown
};
}  // namespace Hooks
SPF_NS_END
