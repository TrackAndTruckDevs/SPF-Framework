#pragma once

#include "SPF/Utils/Signal.hpp"
#include "SPF/SPF_API/SPF_TelemetryData.h" // For the C API data structs
#include "SPF/Telemetry/SCS/Common.hpp"   // For GameState, Timestamps, CommonData
#include "SPF/Telemetry/SCS/Truck.hpp"    // For TruckConstants, TruckData
#include "SPF/Telemetry/SCS/Trailer.hpp"  // For Trailer, TrailerConstants (used in std::vector<Trailer>)
#include "SPF/Telemetry/SCS/Job.hpp"      // For JobConstants, JobData
#include "SPF/Telemetry/SCS/Navigation.hpp" // For NavigationData
#include "SPF/Telemetry/SCS/Controls.hpp" // For Controls
#include "SPF/Telemetry/SCS/Events.hpp"   // For SpecialEvents, GameplayEvents
#include "SPF/Telemetry/SCS/Gearbox.hpp"  // For GearboxConstants
#include <vector> // For std::vector
#include <cstdint>

SPF_NS_BEGIN
namespace Events::Telemetry {

// =================================================================================================
// Telemetry Event Signals
// =================================================================================================
// This file defines the signals used by the EventManager for telemetry-related events.
// These signals are triggered by SCSTelemetryService when it receives new data from the game.
// Other services, like TelemetryApi, can subscribe to these signals to then forward the
// events to plugins that have registered callbacks.

struct TelemetryEventSignals {
    /**
     * @brief Fired when the game state (paused, scale) is updated.
     * @param data The updated game state data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::GameState& data)> OnGameStateUpdated;

    /**
     * @brief Fired when game timestamps are updated.
     * @param data The updated timestamp data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::Timestamps& data)> OnTimestampsUpdated;

    /**
     * @brief Fired when common data (game time, rest stops) is updated.
     * @param data The updated common data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::CommonData& data)> OnCommonDataUpdated;

    /**
     * @brief Fired when the truck's static configuration changes (e.g., new truck).
     * @param data The new truck configuration constants.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::TruckConstants& data)> OnTruckConstantsChanged;
    /**
     * @brief Fired when the list of attached trailers or their data changes.
     * @param data The new trailer configuration constants.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::TrailerConstants& data)> OnTrailerConstantsChanged;

    /**
     * @brief Fired frequently with updated dynamic truck data (speed, RPM, etc.).
     * @param data The updated truck data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::TruckData& data)> OnTruckDataUpdated;

    /**
     * @brief Fired when the list of attached trailers or their data changes.
     * @param trailers A pointer to the array of trailer data.
     * @param count The number of trailers in the array.
     */
    Utils::Signal<void(const std::vector<SPF::Telemetry::SCS::Trailer>& trailers)> OnTrailersUpdated;

    /**
     * @brief Fired when the current job's static configuration changes.
     * @param data The new job configuration constants.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::JobConstants& data)> OnJobConstantsChanged;

    /**
     * @brief Fired when dynamic job data is updated.
     * @param data The updated job data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::JobData& data)> OnJobDataUpdated;

    /**
     * @brief Fired when navigation data is updated.
     * @param data The updated navigation data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::NavigationData& data)> OnNavigationDataUpdated;

    /**
     * @brief Fired when player control inputs are updated.
     * @param data The updated controls data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::Controls& data)> OnControlsUpdated;

    /**
     * @brief Fired when special, single-frame event flags are updated.
     * @param data The updated special events data.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::SpecialEvents& data)> OnSpecialEventsUpdated;

    /**
     * @brief Fired when a gameplay event occurs (e.g., fine, job delivered).
     * @param event_id The string identifier of the event (e.g., "gameplay.fined").
     * @param data The data associated with the event.
     */
    Utils::Signal<void(const char* event_id, const SPF::Telemetry::SCS::GameplayEvents& data)> OnGameplayEventsUpdated;

    /**
     * @brief Fired when the gearbox static configuration changes.
     * @param data The new gearbox configuration constants.
     */
    Utils::Signal<void(const SPF::Telemetry::SCS::GearboxConstants& data)> OnGearboxConstantsChanged;
};

} // namespace Events::Telemetry
SPF_NS_END
