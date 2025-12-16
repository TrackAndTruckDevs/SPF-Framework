#pragma once

#include <vector>

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
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
 * @class TrailerProcessor
 * @brief Processes and owns all trailer-related telemetry data.
 */
class TrailerProcessor {
 public:
  TrailerProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleConfiguration(const scs_telemetry_configuration_t* info);
  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  const std::vector<SCS::Trailer>& GetData() const { return m_trailers; }

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  std::vector<SCS::Trailer> m_trailers;
};

}  // namespace Telemetry
SPF_NS_END
