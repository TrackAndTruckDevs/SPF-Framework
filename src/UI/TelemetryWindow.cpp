#include "SPF/UI/TelemetryWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/ITelemetryService.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
#include "SPF/Telemetry/Sdk.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "fmt/base.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>


SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;
using namespace SPF::Telemetry;
using namespace SPF::Modules;

TelemetryWindow::TelemetryWindow(const std::string& componentName, const std::string& windowId, ITelemetryService& telemetryService)
    : BaseWindow(componentName, windowId),
      m_telemetryService(telemetryService),
      m_gameStateSink(m_telemetryService.GetGameStateSignal()),
      m_timestampsSink(m_telemetryService.GetTimestampsSignal()),
      m_commonDataSink(m_telemetryService.GetCommonDataSignal()),
      m_truckConstantsSink(m_telemetryService.GetTruckConstantsSignal()),
      m_truckDataSink(m_telemetryService.GetTruckDataSignal()),
      m_trailersSink(m_telemetryService.GetTrailersSignal()),
      m_jobConstantsSink(m_telemetryService.GetJobConstantsSignal()),
      m_jobDataSink(m_telemetryService.GetJobDataSignal()),
      m_navigationDataSink(m_telemetryService.GetNavigationDataSignal()),
      m_controlsSink(m_telemetryService.GetControlsSignal()),
      m_specialEventsSink(m_telemetryService.GetSpecialEventsSignal()),
      m_gameplayEventsSink(m_telemetryService.GetGameplayEventsSignal()),
      m_gearboxConstantsSink(m_telemetryService.GetGearboxConstantsSignal()) {
  // Connect signals to their respective slots
  m_gameStateSink.Connect<&TelemetryWindow::OnGameStateUpdate>(this);
  m_timestampsSink.Connect<&TelemetryWindow::OnTimestampsUpdate>(this);
  m_commonDataSink.Connect<&TelemetryWindow::OnCommonDataUpdate>(this);
  m_truckConstantsSink.Connect<&TelemetryWindow::OnTruckConstantsUpdate>(this);
  m_truckDataSink.Connect<&TelemetryWindow::OnTruckDataUpdate>(this);
  m_trailersSink.Connect<&TelemetryWindow::OnTrailersUpdate>(this);
  m_jobConstantsSink.Connect<&TelemetryWindow::OnJobConstantsUpdate>(this);
  m_jobDataSink.Connect<&TelemetryWindow::OnJobDataUpdate>(this);
  m_navigationDataSink.Connect<&TelemetryWindow::OnNavigationDataUpdate>(this);
  m_controlsSink.Connect<&TelemetryWindow::OnControlsUpdate>(this);
  m_specialEventsSink.Connect<&TelemetryWindow::OnSpecialEventsUpdate>(this);
  m_gameplayEventsSink.Connect<&TelemetryWindow::OnGameplayEventUpdate>(this);
  m_gearboxConstantsSink.Connect<&TelemetryWindow::OnGearboxConstantsUpdate>(this);

  m_titleLocalizationKey = "telemetry_window.title";

  RefreshLocalization();
}

void TelemetryWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locTabGame = loc.Get("telemetry_window.tabs.game");
  m_locTabJob = loc.Get("telemetry_window.tabs.job");
  m_locTabNavigation = loc.Get("telemetry_window.tabs.navigation");
  m_locTabTruck = loc.Get("telemetry_window.tabs.truck");
  m_locTabPositioning = loc.Get("telemetry_window.tabs.positioning");
  m_locTabTrailers = loc.Get("telemetry_window.tabs.trailers");
  m_locTabControlsEvents = loc.Get("telemetry_window.tabs.controls_events");

  m_locHeaderGameState = loc.Get("telemetry_window.headers.game_state");
  m_locHeaderConstants = loc.Get("telemetry_window.headers.constants");
  m_locHeaderLiveData = loc.Get("telemetry_window.headers.live_data");
  m_locHeaderPhysics = loc.Get("telemetry_window.headers.physics");
  m_locHeaderWheels = loc.Get("telemetry_window.headers.wheels");
  m_locHeaderDamage = loc.Get("telemetry_window.headers.damage");
  m_locHeaderTruckPositioning = loc.Get("telemetry_window.headers.truck_positioning");
  m_locHeaderControls = loc.Get("telemetry_window.headers.controls");
  m_locHeaderEvents = loc.Get("telemetry_window.headers.events");

  m_locLabelGameTime = loc.Get("telemetry_window.labels.game_time");
  m_locLabelNextRestStop = loc.Get("telemetry_window.labels.next_rest_stop");
  m_locLabelNextRestStopReal = loc.Get("telemetry_window.labels.next_rest_stop_real");
  m_locLabelNextRestStopTime = loc.Get("telemetry_window.labels.next_rest_stop_time");
  m_locLabelPaused = loc.Get("telemetry_window.labels.paused");
  m_locLabelGameId = loc.Get("telemetry_window.labels.game_id");
  m_locLabelLocalScale = loc.Get("telemetry_window.labels.local_scale");
  m_locLabelMultiplayerTimeOffset = loc.Get("telemetry_window.labels.multiplayer_time_offset");
  m_locLabelScsGameVersion = loc.Get("telemetry_window.labels.scs_game_version");
  m_locLabelTelemetryPluginVersion = loc.Get("telemetry_window.labels.telemetry_plugin_version");
  m_locLabelTelemetryGameVersion = loc.Get("telemetry_window.labels.telemetry_game_version");
  m_locLabelGameName = loc.Get("telemetry_window.labels.game_name");
  m_locLabelSubstances = loc.Get("telemetry_window.labels.substances");
  m_locLabelSubstancesNotReceived = loc.Get("telemetry_window.labels.substances_not_received");
  m_locLabelSimulationTime = loc.Get("telemetry_window.labels.simulation_time");
  m_locLabelRenderTime = loc.Get("telemetry_window.labels.render_time");
  m_locLabelPausedSimulationTime = loc.Get("telemetry_window.labels.paused_simulation_time");
  m_locLabelNoActiveJob = loc.Get("telemetry_window.labels.no_active_job");
  m_locLabelContract = loc.Get("telemetry_window.labels.contract");
  m_locLabelMarket = loc.Get("telemetry_window.labels.market");
  m_locLabelIncome = loc.Get("telemetry_window.labels.income");
  m_locLabelPlannedDistance = loc.Get("telemetry_window.labels.planned_distance");
  m_locLabelCargo = loc.Get("telemetry_window.labels.cargo");
  m_locLabelCargoInfo = loc.Get("telemetry_window.labels.cargo_info");
  m_locLabelMass = loc.Get("telemetry_window.labels.mass");
  m_locLabelDamage = loc.Get("telemetry_window.labels.damage");
  m_locLabelLoaded = loc.Get("telemetry_window.labels.loaded");
  m_locLabelSpecialJob = loc.Get("telemetry_window.labels.special_job");
  m_locLabelRoute = loc.Get("telemetry_window.labels.route");
  m_locLabelSource = loc.Get("telemetry_window.labels.source");
  m_locLabelDestination = loc.Get("telemetry_window.labels.destination");
  m_locLabelTime = loc.Get("telemetry_window.labels.time");
  m_locLabelDeliveryDeadline = loc.Get("telemetry_window.labels.delivery_deadline");
  m_locLabelRemainingGameTime = loc.Get("telemetry_window.labels.remaining_game_time");
  m_locLabelSpeedLimit = loc.Get("telemetry_window.labels.speed_limit");
  m_locLabelNextWaypointDist = loc.Get("telemetry_window.labels.next_waypoint_dist");
  m_locLabelNextWaypointTimeGame = loc.Get("telemetry_window.labels.next_waypoint_time_game");
  m_locLabelNextWaypointTimeReal = loc.Get("telemetry_window.labels.next_waypoint_time_real");
  m_locLabelId = loc.Get("telemetry_window.labels.id");
  m_locLabelBrand = loc.Get("telemetry_window.labels.brand");
  m_locLabelName = loc.Get("telemetry_window.labels.name");
  m_locLabelLicensePlate = loc.Get("telemetry_window.labels.license_plate");
  m_locLabelEngineGearbox = loc.Get("telemetry_window.labels.engine_gearbox");
  m_locLabelRpmLimit = loc.Get("telemetry_window.labels.rpm_limit");
  m_locLabelGears = loc.Get("telemetry_window.labels.gears");
  m_locLabelRetarderSteps = loc.Get("telemetry_window.labels.retarder_steps");
  m_locLabelSelectorCount = loc.Get("telemetry_window.labels.selector_count");
  m_locLabelDifferentialRatio = loc.Get("telemetry_window.labels.differential_ratio");
  m_locLabelShifterType = loc.Get("telemetry_window.labels.shifter_type");
  m_locLabelHshifterLayout = loc.Get("telemetry_window.labels.hshifter_layout");
  m_locLabelHshifterSlot = loc.Get("telemetry_window.labels.hshifter_table.slot");
  m_locLabelHshifterGear = loc.Get("telemetry_window.labels.hshifter_table.gear");
  m_locLabelHshifterHandlePos = loc.Get("telemetry_window.labels.hshifter_table.handle_pos");
  m_locLabelHshifterSelectors = loc.Get("telemetry_window.labels.hshifter_table.selectors");
  m_locLabelGearRatios = loc.Get("telemetry_window.labels.gear_ratios");
  m_locLabelForward = loc.Get("telemetry_window.labels.forward");
  m_locLabelReverse = loc.Get("telemetry_window.labels.reverse");
  m_locLabelGearX = loc.Get("telemetry_window.labels.gear_x");
  m_locLabelGearRx = loc.Get("telemetry_window.labels.gear_rx");
  m_locLabelCapacities = loc.Get("telemetry_window.labels.capacities");
  m_locLabelFuelCapacity = loc.Get("telemetry_window.labels.fuel_capacity");
  m_locLabelAdblueCapacity = loc.Get("telemetry_window.labels.adblue_capacity");
  m_locLabelWarningFactors = loc.Get("telemetry_window.labels.warning_factors");
  m_locLabelFuelWarning = loc.Get("telemetry_window.labels.fuel_warning");
  m_locLabelAdblueWarning = loc.Get("telemetry_window.labels.adblue_warning");
  m_locLabelAirPressureWarning = loc.Get("telemetry_window.labels.air_pressure_warning");
  m_locLabelAirPressureEmergency = loc.Get("telemetry_window.labels.air_pressure_emergency");
  m_locLabelOilPressureWarning = loc.Get("telemetry_window.labels.oil_pressure_warning");
  m_locLabelWaterTempWarning = loc.Get("telemetry_window.labels.water_temp_warning");
  m_locLabelBatteryVoltageWarning = loc.Get("telemetry_window.labels.battery_voltage_warning");
  m_locLabelDashboardInfo = loc.Get("telemetry_window.labels.dashboard_info");
  m_locLabelSpeed = loc.Get("telemetry_window.labels.speed");
  m_locLabelEngineRpm = loc.Get("telemetry_window.labels.engine_rpm");
  m_locLabelGear = loc.Get("telemetry_window.labels.gear");
  m_locLabelOdometer = loc.Get("telemetry_window.labels.odometer");
  m_locLabelCruiseControl = loc.Get("telemetry_window.labels.cruise_control");
  m_locLabelFuel = loc.Get("telemetry_window.labels.fuel");
  m_locLabelAdblue = loc.Get("telemetry_window.labels.adblue");
  m_locLabelOil = loc.Get("telemetry_window.labels.oil");
  m_locLabelWaterTemp = loc.Get("telemetry_window.labels.water_temp");
  m_locLabelBatteryVoltage = loc.Get("telemetry_window.labels.battery_voltage");
  m_locLabelDashboardWarnings = loc.Get("telemetry_window.labels.dashboard_warnings");
  m_locLabelFuelWarnState = loc.Get("telemetry_window.labels.fuel_warn_state");
  m_locLabelAdblueWarnState = loc.Get("telemetry_window.labels.adblue_warn_state");
  m_locLabelAirPressureWarnState = loc.Get("telemetry_window.labels.air_pressure_warn_state");
  m_locLabelOilPressureWarnState = loc.Get("telemetry_window.labels.oil_pressure_warn_state");
  m_locLabelWaterTempWarnState = loc.Get("telemetry_window.labels.water_temp_warn_state");
  m_locLabelBatteryVoltageWarnState = loc.Get("telemetry_window.labels.battery_voltage_warn_state");
  m_locLabelSystemStates = loc.Get("telemetry_window.labels.system_states");
  m_locLabelElectricEnabled = loc.Get("telemetry_window.labels.electric_enabled");
  m_locLabelEngineEnabled = loc.Get("telemetry_window.labels.engine_enabled");
  m_locLabelDifferentialLock = loc.Get("telemetry_window.labels.differential_lock");
  m_locLabelWipers = loc.Get("telemetry_window.labels.wipers");
  m_locLabelTruckLiftAxle = loc.Get("telemetry_window.labels.truck_lift_axle");
  m_locLabelTrailerLiftAxle = loc.Get("telemetry_window.labels.trailer_lift_axle");
  m_locLabelLights = loc.Get("telemetry_window.labels.lights");
  m_locLabelBlinkers = loc.Get("telemetry_window.labels.blinkers");
  m_locLabelLightStates = loc.Get("telemetry_window.labels.light_states");
  m_locLabelAuxLights = loc.Get("telemetry_window.labels.aux_lights");
  m_locLabelBrakeReverseLights = loc.Get("telemetry_window.labels.brake_reverse_lights");
  m_locLabelDashboardBacklight = loc.Get("telemetry_window.labels.dashboard_backlight");
  m_locLabelBrakes = loc.Get("telemetry_window.labels.brakes");
  m_locLabelAirPressure = loc.Get("telemetry_window.labels.air_pressure");
  m_locLabelParkingBrake = loc.Get("telemetry_window.labels.parking_brake");
  m_locLabelMotorBrake = loc.Get("telemetry_window.labels.motor_brake");
  m_locLabelRetarderLevel = loc.Get("telemetry_window.labels.retarder_level");
  m_locLabelBrakeTemp = loc.Get("telemetry_window.labels.brake_temp");
  m_locLabelHshifter = loc.Get("telemetry_window.labels.hshifter");
  m_locLabelSlot = loc.Get("telemetry_window.labels.slot");
  m_locLabelSelectors = loc.Get("telemetry_window.labels.selectors");
  m_locLabelLinearVelocity = loc.Get("telemetry_window.labels.linear_velocity");
  m_locLabelAngularVelocity = loc.Get("telemetry_window.labels.angular_velocity");
  m_locLabelLinearAccel = loc.Get("telemetry_window.labels.linear_accel");
  m_locLabelAngularAccel = loc.Get("telemetry_window.labels.angular_accel");
  m_locLabelCabinAngVel = loc.Get("telemetry_window.labels.cabin_ang_vel");
  m_locLabelCabinAngAccel = loc.Get("telemetry_window.labels.cabin_ang_accel");
  m_locLabelWheelX = loc.Get("telemetry_window.labels.wheel_x");
  m_locLabelSubstance = loc.Get("telemetry_window.labels.substance");
  m_locLabelSubstanceUnknown = loc.Get("telemetry_window.labels.substance_unknown");
  m_locLabelOnGround = loc.Get("telemetry_window.labels.on_ground");
  m_locLabelSuspDeflection = loc.Get("telemetry_window.labels.susp_deflection");
  m_locLabelWheelVelocity = loc.Get("telemetry_window.labels.wheel_velocity");
  m_locLabelSteering = loc.Get("telemetry_window.labels.steering");
  m_locLabelRotation = loc.Get("telemetry_window.labels.rotation");
  m_locLabelLift = loc.Get("telemetry_window.labels.lift");
  m_locLabelSteerable = loc.Get("telemetry_window.labels.steerable");
  m_locLabelPowered = loc.Get("telemetry_window.labels.powered");
  m_locLabelLiftable = loc.Get("telemetry_window.labels.liftable");
  m_locLabelSimulated = loc.Get("telemetry_window.labels.simulated");
  m_locLabelRadius = loc.Get("telemetry_window.labels.radius");
  m_locLabelPosition = loc.Get("telemetry_window.labels.position");
  m_locLabelEngineDamage = loc.Get("telemetry_window.labels.engine_damage");
  m_locLabelTransmissionDamage = loc.Get("telemetry_window.labels.transmission_damage");
  m_locLabelCabinDamage = loc.Get("telemetry_window.labels.cabin_damage");
  m_locLabelChassisDamage = loc.Get("telemetry_window.labels.chassis_damage");
  m_locLabelWheelsDamage = loc.Get("telemetry_window.labels.wheels_damage");
  m_locLabelWorldSpace = loc.Get("telemetry_window.labels.world_space");
  m_locLabelWorldPlacement = loc.Get("telemetry_window.labels.world_placement");
  m_locLabelComponentOffsets = loc.Get("telemetry_window.labels.component_offsets");
  m_locLabelCabinOffset = loc.Get("telemetry_window.labels.cabin_offset");
  m_locLabelHeadOffset = loc.Get("telemetry_window.labels.head_offset");
  m_locLabelComponentBasePos = loc.Get("telemetry_window.labels.component_base_pos");
  m_locLabelCabinPos = loc.Get("telemetry_window.labels.cabin_pos");
  m_locLabelHeadPos = loc.Get("telemetry_window.labels.head_pos");
  m_locLabelHookPos = loc.Get("telemetry_window.labels.hook_pos");
  m_locLabelTrailerX = loc.Get("telemetry_window.labels.trailer_x");
  m_locLabelTrailerNa = loc.Get("telemetry_window.labels.trailer_na");
  m_locLabelConnected = loc.Get("telemetry_window.labels.connected");
  m_locLabelGeneral = loc.Get("telemetry_window.labels.general");
  m_locLabelTrailerBrand = loc.Get("telemetry_window.labels.trailer_brand");
  m_locLabelTrailerLicensePlate = loc.Get("telemetry_window.labels.trailer_license_plate");
  m_locLabelBodyType = loc.Get("telemetry_window.labels.body_type");
  m_locLabelChainType = loc.Get("telemetry_window.labels.chain_type");
  m_locLabelCargoAccessoryId = loc.Get("telemetry_window.labels.cargo_accessory_id");
  m_locLabelPhysicsPos = loc.Get("telemetry_window.labels.physics_pos");
  m_locLabelTrailerHookPos = loc.Get("telemetry_window.labels.trailer_hook_pos");
  m_locLabelTrailerWorldPos = loc.Get("telemetry_window.labels.trailer_world_pos");
  m_locLabelTrailerDamageBody = loc.Get("telemetry_window.labels.trailer_damage_body");
  m_locLabelTrailerDamageChassis = loc.Get("telemetry_window.labels.trailer_damage_chassis");
  m_locLabelTrailerDamageWheels = loc.Get("telemetry_window.labels.trailer_damage_wheels");
  m_locLabelTrailerDamageCargo = loc.Get("telemetry_window.labels.trailer_damage_cargo");
  m_locLabelWheelsCount = loc.Get("telemetry_window.labels.wheels_count");
  m_locLabelWheelSuspDeflection = loc.Get("telemetry_window.labels.wheel_susp_deflection");
  m_locLabelWheelAngularVelocity = loc.Get("telemetry_window.labels.wheel_angular_velocity");
  m_locLabelWheelLift = loc.Get("telemetry_window.labels.wheel_lift");
  m_locLabelWheelLiftOffset = loc.Get("telemetry_window.labels.wheel_lift_offset");
  m_locLabelUserInput = loc.Get("telemetry_window.labels.user_input");
  m_locLabelEffectiveInput = loc.Get("telemetry_window.labels.effective_input");
  m_locLabelInputSteering = loc.Get("telemetry_window.labels.input_steering");
  m_locLabelInputThrottle = loc.Get("telemetry_window.labels.input_throttle");
  m_locLabelInputBrake = loc.Get("telemetry_window.labels.input_brake");
  m_locLabelInputClutch = loc.Get("telemetry_window.labels.input_clutch");
  m_locLabelSpecialEvents = loc.Get("telemetry_window.labels.special_events");
  m_locLabelOnJob = loc.Get("telemetry_window.labels.on_job");
  m_locLabelJobCancelled = loc.Get("telemetry_window.labels.job_cancelled");
  m_locLabelJobDelivered = loc.Get("telemetry_window.labels.job_delivered");
  m_locLabelFined = loc.Get("telemetry_window.labels.fined");
  m_locLabelTollgate = loc.Get("telemetry_window.labels.tollgate");
  m_locLabelFerry = loc.Get("telemetry_window.labels.ferry");
  m_locLabelTrain = loc.Get("telemetry_window.labels.train");
  m_locLabelLastGameplayEvent = loc.Get("telemetry_window.labels.last_gameplay_event");
  m_locLabelNoEventYet = loc.Get("telemetry_window.labels.no_event_yet");
  m_locLabelEventJobDelivered = loc.Get("telemetry_window.labels.event_job_delivered");
  m_locLabelEventJobDeliveredDetails = loc.Get("telemetry_window.labels.event_job_delivered_details");
  m_locLabelEventJobDeliveredFlags = loc.Get("telemetry_window.labels.event_job_delivered_flags");
  m_locLabelEventJobCancelled = loc.Get("telemetry_window.labels.event_job_cancelled");
  m_locLabelEventFined = loc.Get("telemetry_window.labels.event_fined");
  m_locLabelEventTollgate = loc.Get("telemetry_window.labels.event_tollgate");
  m_locLabelEventFerry = loc.Get("telemetry_window.labels.event_ferry");
  m_locLabelEventFerryRoute = loc.Get("telemetry_window.labels.event_ferry_route");
  m_locLabelEventFerryRouteTo = loc.Get("telemetry_window.labels.event_ferry_route_to");
  m_locLabelEventTrain = loc.Get("telemetry_window.labels.event_train");
  m_locLabelEventTrainRoute = loc.Get("telemetry_window.labels.event_train_route");
  m_locLabelEventTrainRouteTo = loc.Get("telemetry_window.labels.event_train_route_to");

  m_locDaysOfWeek = {loc.Get("telemetry_window.days_of_week.monday"),
                     loc.Get("telemetry_window.days_of_week.tuesday"),
                     loc.Get("telemetry_window.days_of_week.wednesday"),
                     loc.Get("telemetry_window.days_of_week.thursday"),
                     loc.Get("telemetry_window.days_of_week.friday"),
                     loc.Get("telemetry_window.days_of_week.saturday"),
                     loc.Get("telemetry_window.days_of_week.sunday")};
  m_locFormatDayHourMinute = loc.Get("telemetry_window.formats.day_hour_minute");
  m_locFormatDaysHoursMinutes = loc.Get("telemetry_window.formats.days_hours_minutes");
  m_locFormatHoursMinutes = loc.Get("telemetry_window.formats.hours_minutes");
  m_locFormatRealTimeMinutes = loc.Get("telemetry_window.formats.real_time_minutes");
  m_locFormatRealTimeHoursMinutes = loc.Get("telemetry_window.formats.real_time_hours_minutes");
  m_locFormatNextRestStopTime = loc.Get("telemetry_window.formats.next_rest_stop_time");
  m_locFormatKmH = loc.Get("telemetry_window.formats.km_h");
  m_locFormatMeters = loc.Get("telemetry_window.formats.meters");
  m_locFormatGameTimeSeconds = loc.Get("telemetry_window.formats.game_time_seconds");
  m_locFormatHMS = loc.Get("telemetry_window.formats.h_m_s");
  m_locFormatRealTimeSeconds = loc.Get("telemetry_window.formats.real_time_seconds");
  m_locFormatMS = loc.Get("telemetry_window.formats.m_s");
  m_locFormatGearsFwdRev = loc.Get("telemetry_window.formats.gears_fwd_rev");
  m_locFormatLiters = loc.Get("telemetry_window.formats.liters");
  m_locFormatPercent = loc.Get("telemetry_window.formats.percent");
  m_locFormatPressurePsi = loc.Get("telemetry_window.formats.pressure_psi");
  m_locFormatTempCelsius = loc.Get("telemetry_window.formats.temp_celsius");
  m_locFormatVoltageV = loc.Get("telemetry_window.formats.voltage_v");
  m_locFormatSpeedKmH = loc.Get("telemetry_window.formats.speed_km_h");
  m_locFormatCruiseControlSpeed = loc.Get("telemetry_window.formats.cruise_control_speed");
  m_locFormatFuelConsumption = loc.Get("telemetry_window.formats.fuel_consumption");
  m_locFormatFuelRange = loc.Get("telemetry_window.formats.fuel_range");
  m_locFormatAdblueConsumption = loc.Get("telemetry_window.formats.adblue_consumption");
  m_locFormatOilPressureTemp = loc.Get("telemetry_window.formats.oil_pressure_temp");
  m_locFormatTempCelsiusF = loc.Get("telemetry_window.formats.temp_celsius_f");
  m_locFormatVoltageVF = loc.Get("telemetry_window.formats.voltage_v_f");
  m_locFormatBlinkerState = loc.Get("telemetry_window.formats.blinker_state");
  m_locFormatDashboardBacklight = loc.Get("telemetry_window.formats.dashboard_backlight");
  m_locFormatDamagePercent = loc.Get("telemetry_window.formats.damage_percent");
  m_locFormatVector = loc.Get("telemetry_window.formats.vector");
  m_locFormatPlacementPos = loc.Get("telemetry_window.formats.placement_pos");
  m_locFormatPlacementOri = loc.Get("telemetry_window.formats.placement_ori");
  m_locFormatTrailerWorldPos = loc.Get("telemetry_window.formats.trailer_world_pos");
  m_locFormatDamagePercent2f = loc.Get("telemetry_window.formats.damage_percent_2f");

  m_locGenericYes = loc.Get("telemetry_window.generic.yes");
  m_locGenericNo = loc.Get("telemetry_window.generic.no");
  m_locGenericOn = loc.Get("telemetry_window.generic.on");
  m_locGenericOff = loc.Get("telemetry_window.generic.off");
  m_locGenericWarn = loc.Get("telemetry_window.generic.warn");
  m_locGenericEmergency = loc.Get("telemetry_window.generic.emergency");
  m_locGenericOk = loc.Get("telemetry_window.generic.ok");
  m_locGenericEngaged = loc.Get("telemetry_window.generic.engaged");
  m_locGenericLifted = loc.Get("telemetry_window.generic.lifted");
  m_locGenericDown = loc.Get("telemetry_window.generic.down");
  m_locGenericDimmed = loc.Get("telemetry_window.generic.dimmed");
  m_locGenericFull = loc.Get("telemetry_window.generic.full");
}

// Helper to display a vector
void DisplayFVector(const char* label, const scs_value_fvector_t& vec) { ImGui::Text("%s: (%.2f, %.2f, %.2f)", label, vec.x, vec.y, vec.z); }

// Helper to display fplacement
void DisplayFPlacement(const char* label, const scs_value_fplacement_t& p, const std::string& pos_format, const std::string& ori_format) {
  if (ImGui::TreeNode(label)) {
    ImGui::Text(pos_format.c_str(), p.position.x, p.position.y, p.position.z);
    ImGui::Text(ori_format.c_str(), p.orientation.heading, p.orientation.pitch, p.orientation.roll);
    ImGui::TreePop();
  }
}

// Helper to display dplacement
void DisplayDPlacement(const char* label, const scs_value_dplacement_t& p, const std::string& pos_format, const std::string& ori_format) {
  if (ImGui::TreeNode(label)) {
    ImGui::Text(pos_format.c_str(), p.position.x, p.position.y, p.position.z);
    ImGui::Text(ori_format.c_str(), p.orientation.heading, p.orientation.pitch, p.orientation.roll);
    ImGui::TreePop();
  }
}

void TelemetryWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();

  // Use the cached data which is updated by the signal handlers
  const auto& gameState = m_gameState;
  const auto& timestamps = m_timestamps;
  const auto& commonData = m_commonData;
  const auto& truckConstants = m_truckConstants;
  const auto& truckData = m_truckData;
  const auto& trailers = m_trailers;
  const auto& jobConstants = m_jobConstants;
  const auto& jobData = m_jobData;
  const auto& navigationData = m_navigationData;
  const auto& controls = m_controls;
  const auto& specialEvents = m_specialEvents;
  const auto& gameplayEvents = m_gameplayEvents;
  const auto& gearboxConstants = m_gearboxConstants;
  const auto& lastEventId = m_lastGameplayEventId;

  if (ImGui::BeginTabBar("TelemetryTabs")) {
    if (ImGui::BeginTabItem(m_locTabGame.c_str())) {
      // --- Game State & Time ---
      if (ImGui::CollapsingHeader(m_locHeaderGameState.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto day_to_string = [&](uint32_t day) {
          if (day > 0 && day <= m_locDaysOfWeek.size()) {
            return m_locDaysOfWeek[day - 1].c_str();
          }
          return loc.Get("telemetry_window.days_of_week.unknown").c_str();
        };

        ImGui::Text(m_locLabelGameTime.c_str(), commonData.game_time);
        ImGui::SameLine();
        {
          const uint32_t total_minutes = commonData.game_time;
          const uint32_t minutes_in_day = 24 * 60;
          const uint32_t minutes_in_week = minutes_in_day * 7;
          const uint32_t week_minutes = total_minutes % minutes_in_week;
          const uint32_t day_minutes = total_minutes % minutes_in_day;
          const uint32_t day_of_week = (week_minutes / minutes_in_day) + 1;
          const uint32_t hour = day_minutes / 60;
          const uint32_t minute = day_minutes % 60;
          ImGui::Text(m_locFormatDayHourMinute.c_str(), day_to_string(day_of_week), hour, minute);
        }

        ImGui::Text(m_locLabelNextRestStop.c_str(), commonData.next_rest_stop);
        if (commonData.next_rest_stop >= 0) {
          ImGui::SameLine();
          int total_minutes = commonData.next_rest_stop;
          int total_hours = total_minutes / 60;
          int minutes = total_minutes % 60;
          if (total_hours >= 24) {
            int days = total_hours / 24;
            int hours = total_hours % 24;
            ImGui::Text(m_locFormatDaysHoursMinutes.c_str(), days, hours, minutes);
          } else {
            ImGui::Text(m_locFormatHoursMinutes.c_str(), total_hours, minutes);
          }
        }

        if (commonData.next_rest_stop >= 0) {
          ImGui::Text(m_locLabelNextRestStopReal.c_str(), commonData.next_rest_stop_real_minutes);
          ImGui::SameLine();
          int total_real_minutes = static_cast<int>(commonData.next_rest_stop_real_minutes);
          int real_hours = total_real_minutes / 60;
          int real_minutes = total_real_minutes % 60;
          ImGui::Text(m_locFormatRealTimeHoursMinutes.c_str(), real_hours, real_minutes);
        }

        if (commonData.next_rest_stop >= 0) {
          ImGui::Text(m_locLabelNextRestStopTime.c_str(), day_to_string(commonData.next_rest_stop_time.DayOfWeek), commonData.next_rest_stop_time.Hour, commonData.next_rest_stop_time.Minute);
        }

        ImGui::Separator();
        ImGui::Text(m_locLabelPaused.c_str(), gameState.paused ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelGameName.c_str(), gameState.game_name.c_str());
        ImGui::Text(m_locLabelGameId.c_str(), static_cast<unsigned int>(gameState.game_id));
        ImGui::Text(m_locLabelLocalScale.c_str(), gameState.scale);
        ImGui::Text(m_locLabelMultiplayerTimeOffset.c_str(), gameState.multiplayer_time_offset);
        ImGui::Text(m_locLabelScsGameVersion.c_str(), gameState.scs_game_version_major, gameState.scs_game_version_minor);
        ImGui::Text(m_locLabelTelemetryPluginVersion.c_str(), gameState.telemetry_plugin_version_major, gameState.telemetry_plugin_version_minor);
        ImGui::Text(m_locLabelTelemetryGameVersion.c_str(), gameState.telemetry_game_version_major, gameState.telemetry_game_version_minor);

        if (ImGui::TreeNode(m_locLabelSubstances.c_str())) {
          const auto& substances = commonData.substances;
          if (substances.empty()) {
            ImGui::TextUnformatted(m_locLabelSubstancesNotReceived.c_str());
          } else {
            for (size_t i = 0; i < substances.size(); ++i) {
              ImGui::Text("%zu: %s", i, substances[i].c_str());
            }
          }
          ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text(m_locLabelSimulationTime.c_str(), timestamps.simulation);
        ImGui::Text(m_locLabelRenderTime.c_str(), timestamps.render);
        ImGui::Text(m_locLabelPausedSimulationTime.c_str(), timestamps.paused_simulation);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabJob.c_str())) {
      if (!jobData.on_job) {
        ImGui::TextUnformatted(m_locLabelNoActiveJob.c_str());
      } else {
        ImGui::SeparatorText(m_locLabelContract.c_str());
        ImGui::Text(m_locLabelMarket.c_str(), jobConstants.job_market.c_str());
        ImGui::Text(m_locLabelIncome.c_str(), jobConstants.income);
        ImGui::Text(m_locLabelPlannedDistance.c_str(), jobConstants.planned_distance_km);

        ImGui::SeparatorText(m_locLabelCargo.c_str());
        ImGui::Text(m_locLabelCargoInfo.c_str(), jobConstants.cargo_name.c_str(), jobConstants.cargo_id.c_str());
        ImGui::Text(m_locLabelMass.c_str(), jobConstants.cargo_mass, jobConstants.cargo_unit_count, jobConstants.cargo_unit_mass);
        ImGui::Text(m_locLabelDamage.c_str(), jobData.cargo_damage * 100.0f);
        ImGui::Text(m_locLabelLoaded.c_str(), jobConstants.is_cargo_loaded ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelSpecialJob.c_str(), jobConstants.is_special_job ? m_locGenericYes.c_str() : m_locGenericNo.c_str());

        ImGui::SeparatorText(m_locLabelRoute.c_str());
        ImGui::Text(m_locLabelSource.c_str(), jobConstants.source_company.c_str(), jobConstants.source_company_id.c_str(), jobConstants.source_city.c_str(), jobConstants.source_city_id.c_str());
        ImGui::Text(m_locLabelDestination.c_str(), jobConstants.destination_company.c_str(), jobConstants.destination_company_id.c_str(), jobConstants.destination_city.c_str(), jobConstants.destination_city_id.c_str());

        ImGui::SeparatorText(m_locLabelTime.c_str());
        ImGui::Text(m_locLabelDeliveryDeadline.c_str(), jobConstants.delivery_time);
        ImGui::Text(m_locLabelRemainingGameTime.c_str(), jobData.remaining_delivery_minutes);
        ImGui::SameLine();
        {
          int total_minutes = static_cast<int>(jobData.remaining_delivery_minutes);
          int total_hours = total_minutes / 60;
          if (total_hours >= 24) {
            int days = total_hours / 24;
            int hours = total_hours % 24;
            int minutes = total_minutes % 60;
            ImGui::Text(m_locFormatDaysHoursMinutes.c_str(), days, hours, minutes);
          } else {
            int hours = total_hours;
            int minutes = total_minutes % 60;
            ImGui::Text(m_locFormatHoursMinutes.c_str(), hours, minutes);
          }
        }
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabNavigation.c_str())) {
      ImGui::Text(m_locLabelSpeedLimit.c_str(), navigationData.navigation_speed_limit * 3.6f);
      ImGui::Separator();
      ImGui::Text(m_locLabelNextWaypointDist.c_str(), navigationData.navigation_distance);

      ImGui::Text(m_locLabelNextWaypointTimeGame.c_str(), navigationData.navigation_time);
      ImGui::SameLine();
      {
        int total_seconds = static_cast<int>(navigationData.navigation_time);
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;
        ImGui::Text(m_locFormatHMS.c_str(), hours, minutes, seconds);
      }

      ImGui::Text(m_locLabelNextWaypointTimeReal.c_str(), navigationData.navigation_time_real_seconds);
      ImGui::SameLine();
      {
        int total_seconds = static_cast<int>(navigationData.navigation_time_real_seconds);
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        ImGui::Text(m_locFormatMS.c_str(), minutes, seconds);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabTruck.c_str())) {
      if (ImGui::CollapsingHeader(m_locHeaderConstants.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text(m_locLabelId.c_str(), truckConstants.id.c_str());
        ImGui::Text(m_locLabelBrand.c_str(), truckConstants.brand.c_str(), truckConstants.brand_id.c_str());
        ImGui::Text(m_locLabelName.c_str(), truckConstants.name.c_str());
        ImGui::Text(m_locLabelLicensePlate.c_str(), truckConstants.license_plate.c_str(), truckConstants.license_plate_country.c_str());

        ImGui::SeparatorText(m_locLabelEngineGearbox.c_str());
        ImGui::Text(m_locLabelRpmLimit.c_str(), truckConstants.rpm_limit);
        ImGui::Text(m_locLabelGears.c_str(), truckConstants.forward_gear_count, truckConstants.reverse_gear_count);
        ImGui::Text(m_locLabelRetarderSteps.c_str(), truckConstants.retarder_step_count);
        ImGui::Text(m_locLabelSelectorCount.c_str(), truckData.hshifter_selector.size());
        ImGui::Text(m_locLabelDifferentialRatio.c_str(), truckConstants.differential_ratio);
        ImGui::Text(m_locLabelShifterType.c_str(), gearboxConstants.shifter_type.c_str());

        if (ImGui::TreeNode(m_locLabelHshifterLayout.c_str())) {
          if (ImGui::BeginTable("hshifter_layout", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn(m_locLabelHshifterSlot.c_str());
            ImGui::TableSetupColumn(m_locLabelHshifterGear.c_str());
            ImGui::TableSetupColumn(m_locLabelHshifterHandlePos.c_str());
            ImGui::TableSetupColumn(m_locLabelHshifterSelectors.c_str());
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < gearboxConstants.slot_gear.size(); ++i) {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::Text("%zu", i);
              ImGui::TableSetColumnIndex(1);
              ImGui::Text("%d", gearboxConstants.slot_gear[i]);
              ImGui::TableSetColumnIndex(2);
              ImGui::Text("%u", gearboxConstants.slot_handle_position[i]);
              ImGui::TableSetColumnIndex(3);
              ImGui::Text("0x%X", gearboxConstants.slot_selectors[i]);
            }
            ImGui::EndTable();
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNode(m_locLabelGearRatios.c_str())) {
          if (ImGui::TreeNode(m_locLabelForward.c_str())) {
            for (size_t i = 0; i < truckConstants.gear_ratios_forward.size(); ++i) {
              ImGui::Text(m_locLabelGearX.c_str(), i + 1, truckConstants.gear_ratios_forward[i]);
            }
            ImGui::TreePop();
          }
          if (ImGui::TreeNode(m_locLabelReverse.c_str())) {
            for (size_t i = 0; i < truckConstants.gear_ratios_reverse.size(); ++i) {
              ImGui::Text(m_locLabelGearRx.c_str(), i + 1, truckConstants.gear_ratios_reverse[i]);
            }
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }

        ImGui::SeparatorText(m_locLabelCapacities.c_str());
        ImGui::Text(m_locLabelFuelCapacity.c_str(), truckConstants.fuel_capacity);
        ImGui::Text(m_locLabelAdblueCapacity.c_str(), truckConstants.adblue_capacity);

        ImGui::SeparatorText(m_locLabelWarningFactors.c_str());
        ImGui::Text(m_locLabelFuelWarning.c_str(), truckConstants.fuel_warning_factor * 100.0f);
        ImGui::Text(m_locLabelAdblueWarning.c_str(), truckConstants.adblue_warning_factor * 100.0f);
        ImGui::Text(m_locLabelAirPressureWarning.c_str(), truckConstants.air_pressure_warning);
        ImGui::Text(m_locLabelAirPressureEmergency.c_str(), truckConstants.air_pressure_emergency);
        ImGui::Text(m_locLabelOilPressureWarning.c_str(), truckConstants.oil_pressure_warning);
        ImGui::Text(m_locLabelWaterTempWarning.c_str(), truckConstants.water_temperature_warning);
        ImGui::Text(m_locLabelBatteryVoltageWarning.c_str(), truckConstants.battery_voltage_warning);
      }
      if (ImGui::CollapsingHeader(m_locHeaderLiveData.c_str())) {
        ImGui::SeparatorText(m_locLabelDashboardInfo.c_str());
        ImGui::Text(m_locLabelSpeed.c_str(), truckData.speed * 3.6f);
        ImGui::Text(m_locLabelEngineRpm.c_str(), truckData.engine_rpm);
        ImGui::Text(m_locLabelGear.c_str(), truckData.gear, truckData.displayed_gear);
        ImGui::Text(m_locLabelOdometer.c_str(), truckData.odometer);
        ImGui::Text(m_locLabelCruiseControl.c_str(), truckData.cruise_control_speed > 0.0f ? m_locGenericOn.c_str() : m_locGenericOff.c_str(), truckData.cruise_control_speed * 3.6f);
        ImGui::Text(m_locLabelFuel.c_str(), truckData.fuel_amount, truckData.fuel_average_consumption, truckData.fuel_range);
        ImGui::Text(m_locLabelAdblue.c_str(), truckData.adblue_amount, truckData.adblue_average_consumption);
        ImGui::Text(m_locLabelOil.c_str(), truckData.oil_pressure, truckData.oil_temperature);
        ImGui::Text(m_locLabelWaterTemp.c_str(), truckData.water_temperature);
        ImGui::Text(m_locLabelBatteryVoltage.c_str(), truckData.battery_voltage);

        ImGui::SeparatorText(m_locLabelDashboardWarnings.c_str());
        ImGui::Text(m_locLabelFuelWarnState.c_str(), truckData.fuel_warning ? m_locGenericWarn.c_str() : m_locGenericOk.c_str());
        ImGui::Text(m_locLabelAdblueWarnState.c_str(), truckData.adblue_warning ? m_locGenericWarn.c_str() : m_locGenericOk.c_str());
        ImGui::Text(m_locLabelAirPressureWarnState.c_str(), truckData.air_pressure_warning ? m_locGenericWarn.c_str() : (truckData.air_pressure_emergency ? m_locGenericEmergency.c_str() : m_locGenericOk.c_str()));
        ImGui::Text(m_locLabelOilPressureWarnState.c_str(), truckData.oil_pressure_warning ? m_locGenericWarn.c_str() : m_locGenericOk.c_str());
        ImGui::Text(m_locLabelWaterTempWarnState.c_str(), truckData.water_temperature_warning ? m_locGenericWarn.c_str() : m_locGenericOk.c_str());
        ImGui::Text(m_locLabelBatteryVoltageWarnState.c_str(), truckData.battery_voltage_warning ? m_locGenericWarn.c_str() : m_locGenericOk.c_str());

        ImGui::SeparatorText(m_locLabelSystemStates.c_str());
        ImGui::Text(m_locLabelElectricEnabled.c_str(), truckData.electric_enabled ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelEngineEnabled.c_str(), truckData.engine_enabled ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelDifferentialLock.c_str(), truckData.differential_lock ? m_locGenericEngaged.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelWipers.c_str(), truckData.wipers ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(
          m_locLabelTruckLiftAxle.c_str(), truckData.lift_axle ? m_locGenericLifted.c_str() : m_locGenericDown.c_str(), truckData.lift_axle_indicator ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelTrailerLiftAxle.c_str(),
                    truckData.trailer_lift_axle ? m_locGenericLifted.c_str() : m_locGenericDown.c_str(),
                    truckData.trailer_lift_axle_indicator ? m_locGenericOn.c_str() : m_locGenericOff.c_str());

        ImGui::SeparatorText(m_locLabelLights.c_str());
        ImGui::Text(m_locFormatBlinkerState.c_str(),
                    truckData.lblinker ? m_locGenericOn.c_str() : m_locGenericOff.c_str(),
                    truckData.light_lblinker ? "FLASH" : "_",
                    truckData.rblinker ? m_locGenericOn.c_str() : m_locGenericOff.c_str(),
                    truckData.light_rblinker ? "FLASH" : "_",
                    truckData.hazard_warning ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelLightStates.c_str(),
                    truckData.light_parking ? m_locGenericOn.c_str() : m_locGenericOff.c_str(),
                    truckData.light_low_beam ? m_locGenericOn.c_str() : m_locGenericOff.c_str(),
                    truckData.light_high_beam ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        auto aux_status_to_str = [&](uint32_t status) {
          if (status == 1) return m_locGenericDimmed.c_str();
          if (status == 2) return m_locGenericFull.c_str();
          return m_locGenericOff.c_str();
        };
        ImGui::Text(m_locLabelAuxLights.c_str(), aux_status_to_str(truckData.light_aux_front), aux_status_to_str(truckData.light_aux_roof), truckData.light_beacon ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(
          m_locLabelBrakeReverseLights.c_str(), truckData.light_brake ? m_locGenericOn.c_str() : m_locGenericOff.c_str(), truckData.light_reverse ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelDashboardBacklight.c_str(), truckData.dashboard_backlight);

        ImGui::SeparatorText(m_locLabelBrakes.c_str());
        ImGui::Text(m_locLabelAirPressure.c_str(), truckData.air_pressure);
        ImGui::Text(m_locLabelParkingBrake.c_str(), truckData.parking_brake ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelMotorBrake.c_str(), truckData.motor_brake ? m_locGenericOn.c_str() : m_locGenericOff.c_str());
        ImGui::Text(m_locLabelRetarderLevel.c_str(), truckData.retarder_level);
        ImGui::Text(m_locLabelBrakeTemp.c_str(), truckData.brake_temperature);

        ImGui::SeparatorText(m_locLabelHshifter.c_str());
        ImGui::Text(m_locLabelSlot.c_str(), truckData.hshifter_slot);
        std::string selectors_str;
        for (bool s : truckData.hshifter_selector) {
          selectors_str += s ? "1" : "0";
        }
        ImGui::Text(m_locLabelSelectors.c_str(), selectors_str.c_str());
      }
      if (ImGui::CollapsingHeader(m_locHeaderPhysics.c_str())) {
        DisplayFVector(m_locLabelLinearVelocity.c_str(), truckData.local_linear_velocity);
        DisplayFVector(m_locLabelAngularVelocity.c_str(), truckData.local_angular_velocity);
        DisplayFVector(m_locLabelLinearAccel.c_str(), truckData.local_linear_acceleration);
        DisplayFVector(m_locLabelAngularAccel.c_str(), truckData.local_angular_acceleration);
        ImGui::Separator();
        DisplayFVector(m_locLabelCabinAngVel.c_str(), truckData.cabin_angular_velocity);
        DisplayFVector(m_locLabelCabinAngAccel.c_str(), truckData.cabin_angular_acceleration);
      }
      if (ImGui::CollapsingHeader(m_locHeaderWheels.c_str())) {
        for (size_t i = 0; i < truckData.wheels.size(); ++i) {
          if (i < truckConstants.wheels.size()) {
            const auto& wheel_data = truckData.wheels[i];
            const auto& wheel_const = truckConstants.wheels[i];
            const std::string& wheel_node_format = m_locLabelWheelX;
            std::string wheel_node_id = fmt::format(fmt::runtime(wheel_node_format), i);
            if (ImGui::TreeNode(wheel_node_id.c_str())) {
              const auto& substances = commonData.substances;
              if (wheel_data.substance < substances.size()) {
                ImGui::Text(m_locLabelSubstance.c_str(), substances[wheel_data.substance].c_str(), wheel_data.substance);
              } else {
                ImGui::Text(m_locLabelSubstanceUnknown.c_str(), wheel_data.substance);
              }

              ImGui::Text(m_locLabelOnGround.c_str(), wheel_data.on_ground ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
              ImGui::Text(m_locLabelSuspDeflection.c_str(), wheel_data.suspension_deflection);
              ImGui::Text(m_locLabelWheelVelocity.c_str(), wheel_data.angular_velocity);
              ImGui::Text(m_locLabelSteering.c_str(), wheel_data.steering);
              ImGui::Text(m_locLabelRotation.c_str(), wheel_data.rotation);
              ImGui::Text(m_locLabelLift.c_str(), wheel_data.lift, wheel_data.lift_offset);
              ImGui::Separator();
              ImGui::Text(m_locLabelSteerable.c_str(), wheel_const.steerable ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
              ImGui::Text(m_locLabelPowered.c_str(), wheel_const.powered ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
              ImGui::Text(m_locLabelLiftable.c_str(), wheel_const.liftable ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
              ImGui::Text(m_locLabelSimulated.c_str(), wheel_const.simulated ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
              ImGui::Text(m_locLabelRadius.c_str(), wheel_const.radius);
              DisplayFVector(m_locLabelPosition.c_str(), wheel_const.position);
              ImGui::TreePop();
            }
          }
        }
      }

      if (ImGui::CollapsingHeader(m_locHeaderDamage.c_str())) {
        ImGui::Text(m_locLabelEngineDamage.c_str(), truckData.wear_engine * 100.0f);
        ImGui::Text(m_locLabelTransmissionDamage.c_str(), truckData.wear_transmission * 100.0f);
        ImGui::Text(m_locLabelCabinDamage.c_str(), truckData.wear_cabin * 100.0f);
        ImGui::Text(m_locLabelChassisDamage.c_str(), truckData.wear_chassis * 100.0f);
        ImGui::Text(m_locLabelWheelsDamage.c_str(), truckData.wear_wheels * 100.0f);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabPositioning.c_str())) {
      if (ImGui::CollapsingHeader(m_locHeaderTruckPositioning.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SeparatorText(m_locLabelWorldSpace.c_str());
        DisplayDPlacement(m_locLabelWorldPlacement.c_str(), truckData.world_placement, m_locFormatPlacementPos, m_locFormatPlacementOri);

        ImGui::SeparatorText(m_locLabelComponentOffsets.c_str());
        DisplayFPlacement(m_locLabelCabinOffset.c_str(), truckData.cabin_offset, m_locFormatPlacementPos, m_locFormatPlacementOri);
        DisplayFPlacement(m_locLabelHeadOffset.c_str(), truckData.head_offset, m_locFormatPlacementPos, m_locFormatPlacementOri);

        ImGui::SeparatorText(m_locLabelComponentBasePos.c_str());
        DisplayFVector(m_locLabelCabinPos.c_str(), truckConstants.cabin_position);
        DisplayFVector(m_locLabelHeadPos.c_str(), truckConstants.head_position);
        DisplayFVector(m_locLabelHookPos.c_str(), truckConstants.hook_position);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabTrailers.c_str())) {
      for (size_t i = 0; i < trailers.size(); ++i) {
        const auto& trailer = trailers[i];
        // Skip rendering trailers that are not connected and have no configuration data.
        if (!trailer.data.connected && trailer.constants.id.empty()) continue;

        const std::string& trailer_node_format = m_locLabelTrailerX;
        std::string trailer_node_id = fmt::format(fmt::runtime(trailer_node_format), i, trailer.constants.id.empty() ? m_locLabelTrailerNa.c_str() : trailer.constants.id.c_str());
        if (ImGui::TreeNode(trailer_node_id.c_str())) {
          ImGui::Text(m_locLabelConnected.c_str(), trailer.data.connected ? m_locGenericYes.c_str() : m_locGenericNo.c_str());

          // --- General Info ---
          std::string general_label = m_locLabelGeneral + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(general_label.c_str())) {
            ImGui::Text(m_locLabelId.c_str(), trailer.constants.id.c_str());
            ImGui::Text(m_locLabelName.c_str(), trailer.constants.name.c_str());
            ImGui::Text(m_locLabelTrailerBrand.c_str(), trailer.constants.brand.c_str(), trailer.constants.brand_id.c_str());
            ImGui::Text(m_locLabelTrailerLicensePlate.c_str(), trailer.constants.license_plate.c_str(), trailer.constants.license_plate_country.c_str(), trailer.constants.license_plate_country_id.c_str());
            ImGui::Text(m_locLabelBodyType.c_str(), trailer.constants.body_type.c_str());
            ImGui::Text(m_locLabelChainType.c_str(), trailer.constants.chain_type.c_str());
            ImGui::Text(m_locLabelCargoAccessoryId.c_str(), trailer.constants.cargo_accessory_id.c_str());
            ImGui::TreePop();
          }

          // --- Physics & Position ---
          std::string physics_pos_label = m_locLabelPhysicsPos + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(physics_pos_label.c_str())) {
            DisplayFVector(m_locLabelTrailerHookPos.c_str(), trailer.constants.hook_position);
            ImGui::Text(m_locLabelTrailerWorldPos.c_str(), trailer.data.world_placement.position.x, trailer.data.world_placement.position.y, trailer.data.world_placement.position.z);
            ImGui::Separator();
            DisplayFVector(m_locLabelLinearVelocity.c_str(), trailer.data.local_linear_velocity);
            DisplayFVector(m_locLabelAngularVelocity.c_str(), trailer.data.local_angular_velocity);
            DisplayFVector(m_locLabelLinearAccel.c_str(), trailer.data.local_linear_acceleration);
            DisplayFVector(m_locLabelAngularAccel.c_str(), trailer.data.local_angular_acceleration);
            ImGui::TreePop();
          }

          // --- Damage ---
          std::string damage_label = m_locHeaderDamage + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(damage_label.c_str())) {
            ImGui::Text(m_locLabelTrailerDamageBody.c_str(), trailer.data.wear_body * 100.0f);
            ImGui::Text(m_locLabelTrailerDamageChassis.c_str(), trailer.data.wear_chassis * 100.0f);
            ImGui::Text(m_locLabelTrailerDamageWheels.c_str(), trailer.data.wear_wheels * 100.0f);
            ImGui::Text(m_locLabelTrailerDamageCargo.c_str(), trailer.data.cargo_damage * 100.0f);
            ImGui::TreePop();
          }

          // --- Wheels ---
          std::string wheels_header_str = m_locLabelWheelsCount;
          size_t pos = wheels_header_str.find("{}");
          if (pos != std::string::npos) {
            wheels_header_str.replace(pos, 2, std::to_string(trailer.constants.wheel_count));
          }
          wheels_header_str += "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(wheels_header_str.c_str())) {
            for (size_t j = 0; j < trailer.constants.wheel_count; ++j) {
              if (j >= trailer.data.wheels.size() || j >= trailer.constants.wheels.size()) continue;

              const auto& wheel_data = trailer.data.wheels[j];
              const auto& wheel_const = trailer.constants.wheels[j];
              std::string wheel_node_str = m_locLabelWheelX;
              size_t pos = wheel_node_str.find("{}");
              if (pos != std::string::npos) {
                wheel_node_str.replace(pos, 2, std::to_string(j));
              }
              wheel_node_str += "##Trailer" + std::to_string(i);

              if (ImGui::TreeNode(wheel_node_str.c_str())) {
                // Live Data
                ImGui::Text(m_locLabelOnGround.c_str(), wheel_data.on_ground ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
                ImGui::Text(m_locLabelWheelSuspDeflection.c_str(), wheel_data.suspension_deflection);
                ImGui::Text(m_locLabelWheelAngularVelocity.c_str(), wheel_data.angular_velocity);
                ImGui::Text(m_locLabelSteering.c_str(), wheel_data.steering);
                ImGui::Text(m_locLabelRotation.c_str(), wheel_data.rotation);
                ImGui::Text(m_locLabelWheelLift.c_str(), wheel_data.lift);
                ImGui::Text(m_locLabelWheelLiftOffset.c_str(), wheel_data.lift_offset);
                if (wheel_data.substance < commonData.substances.size()) {
                  ImGui::Text(m_locLabelSubstance.c_str(), commonData.substances[wheel_data.substance].c_str(), wheel_data.substance);
                } else {
                  ImGui::Text(m_locLabelSubstanceUnknown.c_str(), wheel_data.substance);
                }
                ImGui::Separator();
                // Constant Data
                ImGui::Text(m_locLabelRadius.c_str(), wheel_const.radius);
                ImGui::Text(m_locLabelSteerable.c_str(), wheel_const.steerable ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
                ImGui::Text(m_locLabelPowered.c_str(), wheel_const.powered ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
                ImGui::Text(m_locLabelLiftable.c_str(), wheel_const.liftable ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
                ImGui::Text(m_locLabelSimulated.c_str(), wheel_const.simulated ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
                DisplayFVector(m_locLabelPosition.c_str(), wheel_const.position);
                ImGui::TreePop();
              }
            }
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(m_locTabControlsEvents.c_str())) {
      if (ImGui::CollapsingHeader(m_locHeaderControls.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(m_locLabelUserInput.c_str());
        ImGui::Text(m_locLabelInputSteering.c_str(), controls.userInput.steering);
        ImGui::Text(m_locLabelInputThrottle.c_str(), controls.userInput.throttle);
        ImGui::Text(m_locLabelInputBrake.c_str(), controls.userInput.brake);
        ImGui::Text(m_locLabelInputClutch.c_str(), controls.userInput.clutch);
        ImGui::Separator();
        ImGui::TextUnformatted(m_locLabelEffectiveInput.c_str());
        ImGui::Text(m_locLabelInputSteering.c_str(), controls.effectiveInput.steering);
        ImGui::Text(m_locLabelInputThrottle.c_str(), controls.effectiveInput.throttle);
        ImGui::Text(m_locLabelInputBrake.c_str(), controls.effectiveInput.brake);
        ImGui::Text(m_locLabelInputClutch.c_str(), controls.effectiveInput.clutch);
      }
      if (ImGui::CollapsingHeader(m_locHeaderEvents.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(m_locLabelSpecialEvents.c_str());
        ImGui::Text(m_locLabelOnJob.c_str(), jobData.on_job ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelJobCancelled.c_str(), specialEvents.job_cancelled ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelJobDelivered.c_str(), specialEvents.job_delivered ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelFined.c_str(), specialEvents.fined ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelTollgate.c_str(), specialEvents.tollgate ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelFerry.c_str(), specialEvents.ferry ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Text(m_locLabelTrain.c_str(), specialEvents.train ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        ImGui::Separator();
        ImGui::TextUnformatted(m_locLabelLastGameplayEvent.c_str());
        const std::string& lastEventId = m_lastGameplayEventId;
        if (lastEventId.empty()) {
          ImGui::TextUnformatted(m_locLabelNoEventYet.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_job_delivered) {
          const auto& data = gameplayEvents.job_delivered;
          ImGui::Text(m_locLabelEventJobDelivered.c_str(), data.revenue, data.earned_xp, data.cargo_damage * 100.0f);
          ImGui::Text(m_locLabelEventJobDeliveredDetails.c_str(), data.distance_km, data.delivery_time);
          ImGui::Text(
            m_locLabelEventJobDeliveredFlags.c_str(), data.auto_park_used ? m_locGenericYes.c_str() : m_locGenericNo.c_str(), data.auto_load_used ? m_locGenericYes.c_str() : m_locGenericNo.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_job_cancelled) {
          const auto& data = gameplayEvents.job_cancelled;
          ImGui::Text(m_locLabelEventJobCancelled.c_str(), data.penalty);
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_fined) {
          const auto& data = gameplayEvents.player_fined;
          ImGui::Text(m_locLabelEventFined.c_str(), data.fine_amount, data.fine_offence.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_tollgate_paid) {
          const auto& data = gameplayEvents.tollgate_paid;
          ImGui::Text(m_locLabelEventTollgate.c_str(), data.pay_amount);
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_ferry) {
          const auto& data = gameplayEvents.ferry_used;
          ImGui::Text(m_locLabelEventFerry.c_str(), data.pay_amount);
          ImGui::Text(m_locLabelEventFerryRoute.c_str(), data.source_name.c_str(), data.source_id.c_str());
          ImGui::Text(m_locLabelEventFerryRouteTo.c_str(), data.target_name.c_str(), data.target_id.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_train) {
          const auto& data = gameplayEvents.train_used;
          ImGui::Text(m_locLabelEventTrain.c_str(), data.pay_amount);
          ImGui::Text(m_locLabelEventTrainRoute.c_str(), data.source_name.c_str(), data.source_id.c_str());
          ImGui::Text(m_locLabelEventTrainRouteTo.c_str(), data.target_name.c_str(), data.target_id.c_str());
        }
      }
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}

void TelemetryWindow::OnGameStateUpdate(const Telemetry::SCS::GameState& data) { m_gameState = data; }
void TelemetryWindow::OnTimestampsUpdate(const Telemetry::SCS::Timestamps& data) { m_timestamps = data; }
void TelemetryWindow::OnCommonDataUpdate(const Telemetry::SCS::CommonData& data) { m_commonData = data; }
void TelemetryWindow::OnTruckConstantsUpdate(const Telemetry::SCS::TruckConstants& data) { m_truckConstants = data; }
void TelemetryWindow::OnTruckDataUpdate(const Telemetry::SCS::TruckData& data) { m_truckData = data; }
void TelemetryWindow::OnTrailersUpdate(const std::vector<Telemetry::SCS::Trailer>& data) { m_trailers = data; }
void TelemetryWindow::OnJobConstantsUpdate(const Telemetry::SCS::JobConstants& data) { m_jobConstants = data; }
void TelemetryWindow::OnJobDataUpdate(const Telemetry::SCS::JobData& data) { m_jobData = data; }
void TelemetryWindow::OnNavigationDataUpdate(const Telemetry::SCS::NavigationData& data) { m_navigationData = data; }
void TelemetryWindow::OnControlsUpdate(const Telemetry::SCS::Controls& data) { m_controls = data; }
void TelemetryWindow::OnSpecialEventsUpdate(const Telemetry::SCS::SpecialEvents& data) { m_specialEvents = data; }
void TelemetryWindow::OnGameplayEventUpdate(const char* event_id, const Telemetry::SCS::GameplayEvents& data) {
  m_gameplayEvents = data;
  m_lastGameplayEventId = event_id;
}
void TelemetryWindow::OnGearboxConstantsUpdate(const Telemetry::SCS::GearboxConstants& data) { m_gearboxConstants = data; }

}  // namespace UI
SPF_NS_END