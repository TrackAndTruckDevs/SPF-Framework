#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/Sdk.hpp"

SPF_NS_BEGIN

// Forward declarations
namespace Logging {
class Logger;
}
namespace Telemetry {
class GameContext;
}
namespace Events {
class EventManager;
}

namespace Telemetry {
/**
 * @class GameDataProcessor
 * @brief Processes and owns common game state, time, and scale data.
 */
class GameDataProcessor {
 public:
  GameDataProcessor(Logging::Logger& logger, GameContext& context, Events::EventManager& eventManager);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleConfiguration(const scs_telemetry_configuration_t* info);
  void HandlePaused();
  void HandleStarted();
  void HandleFrameStart(const scs_telemetry_frame_start_t* info);
  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  const SCS::GameState& GetGameState() const { return m_gameState; }
  const SCS::Timestamps& GetTimestamps() const { return m_timestamps; }
  const SCS::CommonData& GetCommonData() const { return m_commonData; }

 private:
  void RecalculateRestStopTime();
  void RecalculateRealTimeDurations();

  Logging::Logger& m_logger;
  GameContext& m_context;
  Events::EventManager& m_eventManager;

  bool m_gameWorldReadyNotified = false;

  SCS::GameState m_gameState;
  SCS::Timestamps m_timestamps;
  SCS::CommonData m_commonData;
};

}  // namespace Telemetry
SPF_NS_END
