#include <SPF/Telemetry/GameContext.hpp>

#include <cstring>  // For strcmp

SPF_NS_BEGIN
namespace Telemetry {
GameContext::GameContext(const char* gameId, uint32_t gameVersion) : m_version(gameVersion) {
  if (strcmp(gameId, "eut2") == 0) {
    m_game = Game::ETS2;
  } else if (strcmp(gameId, "ats") == 0) {
    m_game = Game::ATS;
  } else {
    m_game = Game::Unknown;
  }
}

bool GameContext::IsETS2() const { return m_game == Game::ETS2; }

bool GameContext::IsATS() const { return m_game == Game::ATS; }

bool GameContext::IsAdblueSupported() const {
  // AdBlue is specific to ETS2. This might need version checks later.
  return m_game == Game::ETS2;
}

Game GameContext::GetGame() const { return m_game; }

}  // namespace Telemetry
SPF_NS_END
