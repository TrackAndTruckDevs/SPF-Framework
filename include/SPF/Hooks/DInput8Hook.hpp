#pragma once

namespace SPF::Hooks {
struct DInput8Hook {
  static bool Install();
  static void Uninstall();  // Disables the hook for reload
  static void Remove();     // Completely removes the hook for shutdown
};
}  // namespace SPF::Hooks
