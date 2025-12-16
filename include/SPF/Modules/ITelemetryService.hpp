#pragma once

#include <string>
#include <vector>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

// Forward declarations for the new data structures to avoid including the full headers.
// This keeps the interface lightweight.
namespace Telemetry {
namespace SCS {
struct GameState;
struct Timestamps;
struct CommonData;
struct TruckConstants;
struct TruckData;
struct Trailer;
struct JobConstants;
struct JobData;
struct NavigationData;
struct Controls;
struct SpecialEvents;
struct GameplayEvents;
struct GearboxConstants;
}  // namespace SCS
}  // namespace Telemetry

namespace Modules {
/**
 * @class ITelemetryService
 * @brief An abstract interface for providing read-only access to telemetry data.
 *
 * This interface decouples telemetry data consumers (like UI or plugins)
 * from the concrete implementation that provides the data.
 */
class ITelemetryService {
 public:
  virtual ~ITelemetryService() = default;

  // Granular accessors for different data categories
  virtual const Telemetry::SCS::GameState& GetGameState() const = 0;
  virtual const Telemetry::SCS::Timestamps& GetTimestamps() const = 0;
  virtual const Telemetry::SCS::CommonData& GetCommonData() const = 0;
  virtual const Telemetry::SCS::TruckConstants& GetTruckConstants() const = 0;
  virtual const Telemetry::SCS::TruckData& GetTruckData() const = 0;
  virtual const std::vector<Telemetry::SCS::Trailer>& GetTrailers() const = 0;
  virtual const Telemetry::SCS::JobConstants& GetJobConstants() const = 0;
  virtual const Telemetry::SCS::JobData& GetJobData() const = 0;
  virtual const Telemetry::SCS::NavigationData& GetNavigationData() const = 0;
  virtual const Telemetry::SCS::Controls& GetControls() const = 0;
  virtual const Telemetry::SCS::SpecialEvents& GetSpecialEvents() const = 0;
  virtual const Telemetry::SCS::GameplayEvents& GetGameplayEvents() const = 0;
  virtual const Telemetry::SCS::GearboxConstants& GetGearboxConstants() const = 0;
  virtual const std::string& GetLastGameplayEventId() const = 0;

  /**
   * @brief Gets the time elapsed since the last frame.
   * @return Delta time in seconds.
   */
  virtual float GetDeltaTime() const = 0;
};

}  // namespace Modules
SPF_NS_END
