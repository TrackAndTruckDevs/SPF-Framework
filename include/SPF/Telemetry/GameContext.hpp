#pragma once

#include "SPF/Telemetry/Game.hpp"

#include <cstdint>

namespace SPF::Telemetry {
/**
 * @class GameContext
 * @brief Isolates all logic dependent on the current game (ATS/ETS2) and its configuration.
 *
 * Instead of scattering if (game == "ats") throughout the code, we centralize
 * these checks in one place.
 */
class GameContext {
 public:
  GameContext(const char* gameId, uint32_t gameVersion);

  /**
   * @brief Checks if the current game is ETS2.
   */
  bool IsETS2() const;

  /**
   * @brief Checks if the current game is ATS.
   */
  bool IsATS() const;

  /**
   * @brief Checks if the current game/version supports AdBlue.
   */
  bool IsAdblueSupported() const;

  /**
   * @brief Gets the identifier of the current game.
   */
  Game GetGame() const;

 private:
  Game m_game;
  uint32_t m_version;
};

}  // namespace SPF::Telemetry
