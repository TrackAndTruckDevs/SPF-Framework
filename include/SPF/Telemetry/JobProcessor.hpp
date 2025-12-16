#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
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
 * @class JobProcessor
 * @brief Processes and owns job and navigation related telemetry data.
 */
class JobProcessor {
 public:
  JobProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleConfiguration(const scs_telemetry_configuration_t* info);
  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  const SCS::JobConstants& GetJobConstants() const { return m_jobConstants; }
  const SCS::JobData& GetJobData() const { return m_jobData; }
  SCS::JobData& GetMutableJobData() { return m_jobData; }
  const SCS::NavigationData& GetNavigationData() const { return m_navigationData; }
  SCS::NavigationData& GetMutableNavigationData() { return m_navigationData; }

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  SCS::JobConstants m_jobConstants;
  SCS::JobData m_jobData;
  SCS::NavigationData m_navigationData;
};

}  // namespace Telemetry
SPF_NS_END
