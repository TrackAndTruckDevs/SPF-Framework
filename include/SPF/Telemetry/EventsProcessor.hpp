#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/Sdk.hpp"

SPF_NS_BEGIN

// Forward declarations
namespace Logging {
class Logger;
}
namespace Telemetry {
class GameContext;
}

namespace Telemetry {
/**
 * @class EventsProcessor
 * @brief Processes and owns gameplay event related telemetry data.
 */
class EventsProcessor {
 public:
  EventsProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleFrameStart();  // To reset one-frame flags
  void HandleGameplayEvent(const scs_telemetry_gameplay_event_t* info);

  const SCS::SpecialEvents& GetSpecialEvents() const { return m_specialEvents; }
  const SCS::GameplayEvents& GetGameplayEvents() const { return m_gameplayEvents; }
  const std::string& GetLastGameplayEventId() const;

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  std::string m_lastGameplayEventId;
  SCS::SpecialEvents m_specialEvents;
  SCS::GameplayEvents m_gameplayEvents;
};

}  // namespace Telemetry
SPF_NS_END
