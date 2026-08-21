#pragma once

#include "SPF/Namespace.hpp"

#include <algorithm>
#include <utility>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData {

class IWorldScopedService;  // Forward declaration

/**
 * @class WorldServiceRegistry
 * @brief Registry of world-scoped data services participating in the world reload lifecycle.
 *
 * Services self-register from their singleton constructors, so adding a new
 * world-scoped service requires no changes in Core: it automatically joins
 * ResetWorldScopedServices() and FinalizeWorldInitialization().
 */
class WorldServiceRegistry {
 public:
  static WorldServiceRegistry& Get() {
    static WorldServiceRegistry instance;
    return instance;
  }

  WorldServiceRegistry(const WorldServiceRegistry&) = delete;
  void operator=(const WorldServiceRegistry&) = delete;

  /**
   * @brief Adds a service to the world-reload lifecycle.
   * @details Services with a lower priority are finalized first, so
   * dependency roots (e.g. ManagerCoreService) must register with a lower
   * priority than the services that wait on them. Registration is not
   * thread-safe by design: it happens in singleton constructors on the game
   * thread during framework initialization.
   */
  void Register(IWorldScopedService* service, int priority = 100) {
    m_entries.emplace_back(priority, service);
    std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) { return a.first < b.first; });
  }

  /**
   * @brief All registered world-scoped services ordered by priority.
   */
  std::vector<IWorldScopedService*> All() const {
    std::vector<IWorldScopedService*> services;
    services.reserve(m_entries.size());
    for (const auto& [priority, service] : m_entries) {
      services.push_back(service);
    }
    return services;
  }

 private:
  WorldServiceRegistry() = default;

  using Entry = std::pair<int, IWorldScopedService*>;
  std::vector<Entry> m_entries;
};

}  // namespace Data::GameData
SPF_NS_END
