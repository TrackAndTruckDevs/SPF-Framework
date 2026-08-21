#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {

/**
 * @class IWorldScopedService
 * @brief Interface for services whose cached state goes stale when the game world reloads.
 *
 * Implementations self-register into WorldServiceRegistry from their singleton
 * constructor, so Core automatically resets and re-finalizes them on every
 * world load without hardcoding service lists.
 */
class IWorldScopedService {
 public:
  virtual ~IWorldScopedService() = default;

  /**
   * @brief Service name used for lifecycle logging.
   */
  virtual const char* GetName() const = 0;

  /**
   * @brief Clears world-scoped state before the new world is initialized.
   * @details Called on every world reload right after OnWorldUnloading.
   * Implementations decide internally whether that maps to Reset() or
   * Shutdown(); static pattern data may be preserved where it cannot go stale.
   */
  virtual void ResetForWorldReload() = 0;

  /**
   * @brief Re-resolves world data after a (re)load.
   * @details Called on every world load; may be retried on later frames while
   * the game data is not yet available.
   * @return True when the service is fully ready for the current world.
   */
  virtual bool TryFinalizeWorldInit() = 0;
};

}  // namespace Data::GameData
SPF_NS_END
