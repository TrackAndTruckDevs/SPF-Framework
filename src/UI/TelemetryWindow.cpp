#include "SPF/UI/TelemetryWindow.hpp"
#include "SPF/Modules/ITelemetryService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"

#include <imgui.h>
#include <fmt/core.h>
#include <string>

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

  m_locTabGame = "telemetry_window.tabs.game";
  m_locTabJob = "telemetry_window.tabs.job";
  m_locTabNavigation = "telemetry_window.tabs.navigation";
  m_locTabTruck = "telemetry_window.tabs.truck";
  m_locTabPositioning = "telemetry_window.tabs.positioning";
  m_locTabTrailers = "telemetry_window.tabs.trailers";
  m_locTabControlsEvents = "telemetry_window.tabs.controls_events";

  m_locHeaderGameState = "telemetry_window.headers.game_state";
  m_locHeaderConstants = "telemetry_window.headers.constants";
  m_locHeaderLiveData = "telemetry_window.headers.live_data";
  m_locHeaderPhysics = "telemetry_window.headers.physics";
  m_locHeaderWheels = "telemetry_window.headers.wheels";
  m_locHeaderDamage = "telemetry_window.headers.damage";
  m_locHeaderTruckPositioning = "telemetry_window.headers.truck_positioning";
  m_locHeaderControls = "telemetry_window.headers.controls";
  m_locHeaderEvents = "telemetry_window.headers.events";

  m_locLabelGameTime = "telemetry_window.labels.game_time";
  m_locLabelNextRestStop = "telemetry_window.labels.next_rest_stop";
  m_locLabelNextRestStopReal = "telemetry_window.labels.next_rest_stop_real";
  m_locLabelNextRestStopTime = "telemetry_window.labels.next_rest_stop_time";
  m_locLabelPaused = "telemetry_window.labels.paused";
  m_locLabelGameId = "telemetry_window.labels.game_id";
  m_locLabelLocalScale = "telemetry_window.labels.local_scale";
  m_locLabelMultiplayerTimeOffset = "telemetry_window.labels.multiplayer_time_offset";
  m_locLabelScsGameVersion = "telemetry_window.labels.scs_game_version";
  m_locLabelTelemetryPluginVersion = "telemetry_window.labels.telemetry_plugin_version";
  m_locLabelTelemetryGameVersion = "telemetry_window.labels.telemetry_game_version";
  m_locLabelGameName = "telemetry_window.labels.game_name";
  m_locLabelSubstances = "telemetry_window.labels.substances";
  m_locLabelSubstancesNotReceived = "telemetry_window.labels.substances_not_received";
  m_locLabelSimulationTime = "telemetry_window.labels.simulation_time";
  m_locLabelRenderTime = "telemetry_window.labels.render_time";
  m_locLabelPausedSimulationTime = "telemetry_window.labels.paused_simulation_time";
  m_locLabelNoActiveJob = "telemetry_window.labels.no_active_job";
  m_locLabelContract = "telemetry_window.labels.contract";
  m_locLabelMarket = "telemetry_window.labels.market";
  m_locLabelIncome = "telemetry_window.labels.income";
  m_locLabelPlannedDistance = "telemetry_window.labels.planned_distance";
  m_locLabelCargo = "telemetry_window.labels.cargo";
  m_locLabelCargoInfo = "telemetry_window.labels.cargo_info";
  m_locLabelMass = "telemetry_window.labels.mass";
  m_locLabelDamage = "telemetry_window.labels.damage";
  m_locLabelLoaded = "telemetry_window.labels.loaded";
  m_locLabelSpecialJob = "telemetry_window.labels.special_job";
  m_locLabelRoute = "telemetry_window.labels.route";
  m_locLabelSource = "telemetry_window.labels.source";
  m_locLabelDestination = "telemetry_window.labels.destination";
  m_locLabelTime = "telemetry_window.labels.time";
  m_locLabelDeliveryDeadline = "telemetry_window.labels.delivery_deadline";
  m_locLabelRemainingGameTime = "telemetry_window.labels.remaining_game_time";
  m_locLabelSpeedLimit = "telemetry_window.labels.speed_limit";
  m_locLabelNextWaypointDist = "telemetry_window.labels.next_waypoint_dist";
  m_locLabelNextWaypointTimeGame = "telemetry_window.labels.next_waypoint_time_game";
  m_locLabelNextWaypointTimeReal = "telemetry_window.labels.next_waypoint_time_real";
  m_locLabelId = "telemetry_window.labels.id";
  m_locLabelBrand = "telemetry_window.labels.brand";
  m_locLabelName = "telemetry_window.labels.name";
  m_locLabelLicensePlate = "telemetry_window.labels.license_plate";
  m_locLabelEngineGearbox = "telemetry_window.labels.engine_gearbox";
  m_locLabelRpmLimit = "telemetry_window.labels.rpm_limit";
  m_locLabelGears = "telemetry_window.labels.gears";
  m_locLabelRetarderSteps = "telemetry_window.labels.retarder_steps";
  m_locLabelSelectorCount = "telemetry_window.labels.selector_count";
  m_locLabelDifferentialRatio = "telemetry_window.labels.differential_ratio";
  m_locLabelShifterType = "telemetry_window.labels.shifter_type";
  m_locLabelHshifterLayout = "telemetry_window.labels.hshifter_layout";
  m_locLabelHshifterSlot = "telemetry_window.labels.hshifter_table.slot";
  m_locLabelHshifterGear = "telemetry_window.labels.hshifter_table.gear";
  m_locLabelHshifterHandlePos = "telemetry_window.labels.hshifter_table.handle_pos";
  m_locLabelHshifterSelectors = "telemetry_window.labels.hshifter_table.selectors";
  m_locLabelGearRatios = "telemetry_window.labels.gear_ratios";
  m_locLabelForward = "telemetry_window.labels.forward";
  m_locLabelReverse = "telemetry_window.labels.reverse";
  m_locLabelGearX = "telemetry_window.labels.gear_x";
  m_locLabelGearRx = "telemetry_window.labels.gear_rx";
  m_locLabelCapacities = "telemetry_window.labels.capacities";
  m_locLabelFuelCapacity = "telemetry_window.labels.fuel_capacity";
  m_locLabelAdblueCapacity = "telemetry_window.labels.adblue_capacity";
  m_locLabelWarningFactors = "telemetry_window.labels.warning_factors";
  m_locLabelFuelWarning = "telemetry_window.labels.fuel_warning";
  m_locLabelAdblueWarning = "telemetry_window.labels.adblue_warning";
  m_locLabelAirPressureWarning = "telemetry_window.labels.air_pressure_warning";
  m_locLabelAirPressureEmergency = "telemetry_window.labels.air_pressure_emergency";
  m_locLabelOilPressureWarning = "telemetry_window.labels.oil_pressure_warning";
  m_locLabelWaterTempWarning = "telemetry_window.labels.water_temp_warning";
  m_locLabelBatteryVoltageWarning = "telemetry_window.labels.battery_voltage_warning";
  m_locLabelDashboardInfo = "telemetry_window.labels.dashboard_info";
  m_locLabelSpeed = "telemetry_window.labels.speed";
  m_locLabelEngineRpm = "telemetry_window.labels.engine_rpm";
  m_locLabelGear = "telemetry_window.labels.gear";
  m_locLabelOdometer = "telemetry_window.labels.odometer";
  m_locLabelCruiseControl = "telemetry_window.labels.cruise_control";
  m_locLabelFuel = "telemetry_window.labels.fuel";
  m_locLabelAdblue = "telemetry_window.labels.adblue";
  m_locLabelOil = "telemetry_window.labels.oil";
  m_locLabelWaterTemp = "telemetry_window.labels.water_temp";
  m_locLabelBatteryVoltage = "telemetry_window.labels.battery_voltage";
  m_locLabelDashboardWarnings = "telemetry_window.labels.dashboard_warnings";
  m_locLabelFuelWarnState = "telemetry_window.labels.fuel_warn_state";
  m_locLabelAdblueWarnState = "telemetry_window.labels.adblue_warn_state";
  m_locLabelAirPressureWarnState = "telemetry_window.labels.air_pressure_warn_state";
  m_locLabelOilPressureWarnState = "telemetry_window.labels.oil_pressure_warn_state";
  m_locLabelWaterTempWarnState = "telemetry_window.labels.water_temp_warn_state";
  m_locLabelBatteryVoltageWarnState = "telemetry_window.labels.battery_voltage_warn_state";
  m_locLabelSystemStates = "telemetry_window.labels.system_states";
  m_locLabelElectricEnabled = "telemetry_window.labels.electric_enabled";
  m_locLabelEngineEnabled = "telemetry_window.labels.engine_enabled";
  m_locLabelDifferentialLock = "telemetry_window.labels.differential_lock";
  m_locLabelWipers = "telemetry_window.labels.wipers";
  m_locLabelTruckLiftAxle = "telemetry_window.labels.truck_lift_axle";
  m_locLabelTrailerLiftAxle = "telemetry_window.labels.trailer_lift_axle";
  m_locLabelLights = "telemetry_window.labels.lights";
  m_locLabelBlinkers = "telemetry_window.labels.blinkers";
  m_locLabelLightStates = "telemetry_window.labels.light_states";
  m_locLabelAuxLights = "telemetry_window.labels.aux_lights";
  m_locLabelBrakeReverseLights = "telemetry_window.labels.brake_reverse_lights";
  m_locLabelDashboardBacklight = "telemetry_window.labels.dashboard_backlight";
  m_locLabelBrakes = "telemetry_window.labels.brakes";
  m_locLabelAirPressure = "telemetry_window.labels.air_pressure";
  m_locLabelParkingBrake = "telemetry_window.labels.parking_brake";
  m_locLabelMotorBrake = "telemetry_window.labels.motor_brake";
  m_locLabelRetarderLevel = "telemetry_window.labels.retarder_level";
  m_locLabelBrakeTemp = "telemetry_window.labels.brake_temp";
  m_locLabelHshifter = "telemetry_window.labels.hshifter";
  m_locLabelSlot = "telemetry_window.labels.slot";
  m_locLabelSelectors = "telemetry_window.labels.selectors";
  m_locLabelLinearVelocity = "telemetry_window.labels.linear_velocity";
  m_locLabelAngularVelocity = "telemetry_window.labels.angular_velocity";
  m_locLabelLinearAccel = "telemetry_window.labels.linear_accel";
  m_locLabelAngularAccel = "telemetry_window.labels.angular_accel";
  m_locLabelCabinAngVel = "telemetry_window.labels.cabin_ang_vel";
  m_locLabelCabinAngAccel = "telemetry_window.labels.cabin_ang_accel";
  m_locLabelWheelX = "telemetry_window.labels.wheel_x";
  m_locLabelSubstance = "telemetry_window.labels.substance";
  m_locLabelSubstanceUnknown = "telemetry_window.labels.substance_unknown";
  m_locLabelOnGround = "telemetry_window.labels.on_ground";
  m_locLabelSuspDeflection = "telemetry_window.labels.susp_deflection";
  m_locLabelWheelVelocity = "telemetry_window.labels.wheel_velocity";
  m_locLabelSteering = "telemetry_window.labels.steering";
  m_locLabelRotation = "telemetry_window.labels.rotation";
  m_locLabelLift = "telemetry_window.labels.lift";
  m_locLabelSteerable = "telemetry_window.labels.steerable";
  m_locLabelPowered = "telemetry_window.labels.powered";
  m_locLabelLiftable = "telemetry_window.labels.liftable";
  m_locLabelSimulated = "telemetry_window.labels.simulated";
  m_locLabelRadius = "telemetry_window.labels.radius";
  m_locLabelPosition = "telemetry_window.labels.position";
  m_locLabelEngineDamage = "telemetry_window.labels.engine_damage";
  m_locLabelTransmissionDamage = "telemetry_window.labels.transmission_damage";
  m_locLabelCabinDamage = "telemetry_window.labels.cabin_damage";
  m_locLabelChassisDamage = "telemetry_window.labels.chassis_damage";
  m_locLabelWheelsDamage = "telemetry_window.labels.wheels_damage";
  m_locLabelWorldSpace = "telemetry_window.labels.world_space";
  m_locLabelWorldPlacement = "telemetry_window.labels.world_placement";
  m_locLabelComponentOffsets = "telemetry_window.labels.component_offsets";
  m_locLabelCabinOffset = "telemetry_window.labels.cabin_offset";
  m_locLabelHeadOffset = "telemetry_window.labels.head_offset";
  m_locLabelComponentBasePos = "telemetry_window.labels.component_base_pos";
  m_locLabelCabinPos = "telemetry_window.labels.cabin_pos";
  m_locLabelHeadPos = "telemetry_window.labels.head_pos";
  m_locLabelHookPos = "telemetry_window.labels.hook_pos";
  m_locLabelTrailerX = "telemetry_window.labels.trailer_x";
  m_locLabelTrailerNa = "telemetry_window.labels.trailer_na";
  m_locLabelConnected = "telemetry_window.labels.connected";
  m_locLabelGeneral = "telemetry_window.labels.general";
  m_locLabelTrailerBrand = "telemetry_window.labels.trailer_brand";
  m_locLabelTrailerLicensePlate = "telemetry_window.labels.trailer_license_plate";
  m_locLabelBodyType = "telemetry_window.labels.body_type";
  m_locLabelChainType = "telemetry_window.labels.chain_type";
  m_locLabelCargoAccessoryId = "telemetry_window.labels.cargo_accessory_id";
  m_locLabelPhysicsPos = "telemetry_window.labels.physics_pos";
  m_locLabelTrailerHookPos = "telemetry_window.labels.trailer_hook_pos";
  m_locLabelTrailerWorldPos = "telemetry_window.labels.trailer_world_pos";
  m_locLabelTrailerDamageBody = "telemetry_window.labels.trailer_damage_body";
  m_locLabelTrailerDamageChassis = "telemetry_window.labels.trailer_damage_chassis";
  m_locLabelTrailerDamageWheels = "telemetry_window.labels.trailer_damage_wheels";
  m_locLabelTrailerDamageCargo = "telemetry_window.labels.trailer_damage_cargo";
  m_locLabelWheelsCount = "telemetry_window.labels.wheels_count";
  m_locLabelWheelSuspDeflection = "telemetry_window.labels.wheel_susp_deflection";
  m_locLabelWheelAngularVelocity = "telemetry_window.labels.wheel_angular_velocity";
  m_locLabelWheelLift = "telemetry_window.labels.wheel_lift";
  m_locLabelWheelLiftOffset = "telemetry_window.labels.wheel_lift_offset";
  m_locLabelUserInput = "telemetry_window.labels.user_input";
  m_locLabelEffectiveInput = "telemetry_window.labels.effective_input";
  m_locLabelInputSteering = "telemetry_window.labels.input_steering";
  m_locLabelInputThrottle = "telemetry_window.labels.input_throttle";
  m_locLabelInputBrake = "telemetry_window.labels.input_brake";
  m_locLabelInputClutch = "telemetry_window.labels.input_clutch";
  m_locLabelSpecialEvents = "telemetry_window.labels.special_events";
  m_locLabelOnJob = "telemetry_window.labels.on_job";
  m_locLabelJobCancelled = "telemetry_window.labels.job_cancelled";
  m_locLabelJobDelivered = "telemetry_window.labels.job_delivered";
  m_locLabelFined = "telemetry_window.labels.fined";
  m_locLabelTollgate = "telemetry_window.labels.tollgate";
  m_locLabelFerry = "telemetry_window.labels.ferry";
  m_locLabelTrain = "telemetry_window.labels.train";
  m_locLabelLastGameplayEvent = "telemetry_window.labels.last_gameplay_event";
  m_locLabelNoEventYet = "telemetry_window.labels.no_event_yet";
  m_locLabelEventJobDelivered = "telemetry_window.labels.event_job_delivered";
  m_locLabelEventJobDeliveredDetails = "telemetry_window.labels.event_job_delivered_details";
  m_locLabelEventJobDeliveredFlags = "telemetry_window.labels.event_job_delivered_flags";
  m_locLabelEventJobCancelled = "telemetry_window.labels.event_job_cancelled";
  m_locLabelEventFined = "telemetry_window.labels.event_fined";
  m_locLabelEventTollgate = "telemetry_window.labels.event_tollgate";
  m_locLabelEventFerry = "telemetry_window.labels.event_ferry";
  m_locLabelEventFerryRoute = "telemetry_window.labels.event_ferry_route";
  m_locLabelEventFerryRouteTo = "telemetry_window.labels.event_ferry_route_to";
  m_locLabelEventTrain = "telemetry_window.labels.event_train";
  m_locLabelEventTrainRoute = "telemetry_window.labels.event_train_route";
  m_locLabelEventTrainRouteTo = "telemetry_window.labels.event_train_route_to";

  m_locDaysOfWeek = {"telemetry_window.days_of_week.monday",   "telemetry_window.days_of_week.tuesday", "telemetry_window.days_of_week.wednesday",
                     "telemetry_window.days_of_week.thursday", "telemetry_window.days_of_week.friday",  "telemetry_window.days_of_week.saturday",
                     "telemetry_window.days_of_week.sunday"};
  m_locFormatDayHourMinute = "telemetry_window.formats.day_hour_minute";
  m_locFormatDaysHoursMinutes = "telemetry_window.formats.days_hours_minutes";
  m_locFormatHoursMinutes = "telemetry_window.formats.hours_minutes";
  m_locFormatRealTimeMinutes = "telemetry_window.formats.real_time_minutes";
  m_locFormatRealTimeHoursMinutes = "telemetry_window.formats.real_time_hours_minutes";
  m_locFormatNextRestStopTime = "telemetry_window.formats.next_rest_stop_time";
  m_locFormatKmH = "telemetry_window.formats.km_h";
  m_locFormatMeters = "telemetry_window.formats.meters";
  m_locFormatGameTimeSeconds = "telemetry_window.formats.game_time_seconds";
  m_locFormatHMS = "telemetry_window.formats.h_m_s";
  m_locFormatRealTimeSeconds = "telemetry_window.formats.real_time_seconds";
  m_locFormatMS = "telemetry_window.formats.m_s";
  m_locFormatGearsFwdRev = "telemetry_window.formats.gears_fwd_rev";
  m_locFormatLiters = "telemetry_window.formats.liters";
  m_locFormatPercent = "telemetry_window.formats.percent";
  m_locFormatPressurePsi = "telemetry_window.formats.pressure_psi";
  m_locFormatTempCelsius = "telemetry_window.formats.temp_celsius";
  m_locFormatVoltageV = "telemetry_window.formats.voltage_v";
  m_locFormatSpeedKmH = "telemetry_window.formats.speed_km_h";
  m_locFormatCruiseControlSpeed = "telemetry_window.formats.cruise_control_speed";
  m_locFormatFuelConsumption = "telemetry_window.formats.fuel_consumption";
  m_locFormatFuelRange = "telemetry_window.formats.fuel_range";
  m_locFormatAdblueConsumption = "telemetry_window.formats.adblue_consumption";
  m_locFormatOilPressureTemp = "telemetry_window.formats.oil_pressure_temp";
  m_locFormatTempCelsiusF = "telemetry_window.formats.temp_celsius_f";
  m_locFormatVoltageVF = "telemetry_window.formats.voltage_v_f";
  m_locFormatBlinkerState = "telemetry_window.formats.blinker_state";
  m_locFormatDashboardBacklight = "telemetry_window.formats.dashboard_backlight";
  m_locFormatDamagePercent = "telemetry_window.formats.damage_percent";
  m_locFormatVector = "telemetry_window.formats.vector";
  m_locFormatPlacementPos = "telemetry_window.formats.placement_pos";
  m_locFormatPlacementOri = "telemetry_window.formats.placement_ori";
  m_locFormatTrailerWorldPos = "telemetry_window.formats.trailer_world_pos";
  m_locFormatDamagePercent2f = "telemetry_window.formats.damage_percent_2f";

  m_locGenericYes = "telemetry_window.generic.yes";
  m_locGenericNo = "telemetry_window.generic.no";
  m_locGenericOn = "telemetry_window.generic.on";
  m_locGenericOff = "telemetry_window.generic.off";
  m_locGenericWarn = "telemetry_window.generic.warn";
  m_locGenericEmergency = "telemetry_window.generic.emergency";
  m_locGenericOk = "telemetry_window.generic.ok";
  m_locGenericEngaged = "telemetry_window.generic.engaged";
  m_locGenericLifted = "telemetry_window.generic.lifted";
  m_locGenericDown = "telemetry_window.generic.down";
  m_locGenericDimmed = "telemetry_window.generic.dimmed";
  m_locGenericFull = "telemetry_window.generic.full";
}

const char* TelemetryWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str(); }

// Helper to display a vector
void DisplayFVector(const char* label, const scs_value_fvector_t& vec) {
  ImGui::Text("%s: (%.2f, %.2f, %.2f)", label, vec.x, vec.y, vec.z);
}

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
    if (ImGui::BeginTabItem(loc.Get(m_locTabGame).c_str())) {
      // --- Game State & Time ---
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderGameState).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto day_to_string = [&](uint32_t day) {
          if (day > 0 && day <= m_locDaysOfWeek.size()) {
            return loc.Get(m_locDaysOfWeek[day - 1]).c_str();
          }
          return loc.Get("telemetry_window.days_of_week.unknown").c_str();
        };

        ImGui::Text(loc.Get(m_locLabelGameTime).c_str(), commonData.game_time);
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
          ImGui::Text(loc.Get(m_locFormatDayHourMinute).c_str(), day_to_string(day_of_week), hour, minute);
        }

        ImGui::Text(loc.Get(m_locLabelNextRestStop).c_str(), commonData.next_rest_stop);
        if (commonData.next_rest_stop >= 0) {
          ImGui::SameLine();
          int total_minutes = commonData.next_rest_stop;
          int total_hours = total_minutes / 60;
          int minutes = total_minutes % 60;
          if (total_hours >= 24) {
            int days = total_hours / 24;
            int hours = total_hours % 24;
            ImGui::Text(loc.Get(m_locFormatDaysHoursMinutes).c_str(), days, hours, minutes);
          } else {
            ImGui::Text(loc.Get(m_locFormatHoursMinutes).c_str(), total_hours, minutes);
          }
        }

        if (commonData.next_rest_stop >= 0) {
          ImGui::Text(loc.Get(m_locLabelNextRestStopReal).c_str(), commonData.next_rest_stop_real_minutes);
          ImGui::SameLine();
          int total_real_minutes = static_cast<int>(commonData.next_rest_stop_real_minutes);
          int real_hours = total_real_minutes / 60;
          int real_minutes = total_real_minutes % 60;
          ImGui::Text(loc.Get(m_locFormatRealTimeHoursMinutes).c_str(), real_hours, real_minutes);
        }

        if (commonData.next_rest_stop >= 0) {
          ImGui::Text(loc.Get(m_locLabelNextRestStopTime).c_str(),
                      day_to_string(commonData.next_rest_stop_time.DayOfWeek),
                      commonData.next_rest_stop_time.Hour,
                      commonData.next_rest_stop_time.Minute);
        }

        ImGui::Separator();
        ImGui::Text(loc.Get(m_locLabelPaused).c_str(), gameState.paused ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelGameName).c_str(), gameState.game_name.c_str());
        ImGui::Text(loc.Get(m_locLabelGameId).c_str(), static_cast<unsigned int>(gameState.game_id));
        ImGui::Text(loc.Get(m_locLabelLocalScale).c_str(), gameState.scale);
        ImGui::Text(loc.Get(m_locLabelMultiplayerTimeOffset).c_str(), gameState.multiplayer_time_offset);
        ImGui::Text(loc.Get(m_locLabelScsGameVersion).c_str(), gameState.scs_game_version_major, gameState.scs_game_version_minor);
        ImGui::Text(loc.Get(m_locLabelTelemetryPluginVersion).c_str(), gameState.telemetry_plugin_version_major, gameState.telemetry_plugin_version_minor);
        ImGui::Text(loc.Get(m_locLabelTelemetryGameVersion).c_str(), gameState.telemetry_game_version_major, gameState.telemetry_game_version_minor);

        if (ImGui::TreeNode(loc.Get(m_locLabelSubstances).c_str())) {
          const auto& substances = commonData.substances;
          if (substances.empty()) {
            ImGui::TextUnformatted(loc.Get(m_locLabelSubstancesNotReceived).c_str());
          } else {
            for (size_t i = 0; i < substances.size(); ++i) {
              ImGui::Text("%zu: %s", i, substances[i].c_str());
            }
          }
          ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text(loc.Get(m_locLabelSimulationTime).c_str(), timestamps.simulation);
        ImGui::Text(loc.Get(m_locLabelRenderTime).c_str(), timestamps.render);
        ImGui::Text(loc.Get(m_locLabelPausedSimulationTime).c_str(), timestamps.paused_simulation);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabJob).c_str())) {
      if (!jobData.on_job) {
        ImGui::TextUnformatted(loc.Get(m_locLabelNoActiveJob).c_str());
      } else {
        ImGui::SeparatorText(loc.Get(m_locLabelContract).c_str());
        ImGui::Text(loc.Get(m_locLabelMarket).c_str(), jobConstants.job_market.c_str());
        ImGui::Text(loc.Get(m_locLabelIncome).c_str(), jobConstants.income);
        ImGui::Text(loc.Get(m_locLabelPlannedDistance).c_str(), jobConstants.planned_distance_km);

        ImGui::SeparatorText(loc.Get(m_locLabelCargo).c_str());
        ImGui::Text(loc.Get(m_locLabelCargoInfo).c_str(), jobConstants.cargo_name.c_str(), jobConstants.cargo_id.c_str());
        ImGui::Text(loc.Get(m_locLabelMass).c_str(), jobConstants.cargo_mass, jobConstants.cargo_unit_count, jobConstants.cargo_unit_mass);
        ImGui::Text(loc.Get(m_locLabelDamage).c_str(), jobData.cargo_damage * 100.0f);
        ImGui::Text(loc.Get(m_locLabelLoaded).c_str(), jobConstants.is_cargo_loaded ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelSpecialJob).c_str(), jobConstants.is_special_job ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());

        ImGui::SeparatorText(loc.Get(m_locLabelRoute).c_str());
        ImGui::Text(loc.Get(m_locLabelSource).c_str(),
                    jobConstants.source_company.c_str(),
                    jobConstants.source_company_id.c_str(),
                    jobConstants.source_city.c_str(),
                    jobConstants.source_city_id.c_str());
        ImGui::Text(loc.Get(m_locLabelDestination).c_str(),
                    jobConstants.destination_company.c_str(),
                    jobConstants.destination_company_id.c_str(),
                    jobConstants.destination_city.c_str(),
                    jobConstants.destination_city_id.c_str());

        ImGui::SeparatorText(loc.Get(m_locLabelTime).c_str());
        ImGui::Text(loc.Get(m_locLabelDeliveryDeadline).c_str(), jobConstants.delivery_time);
        ImGui::Text(loc.Get(m_locLabelRemainingGameTime).c_str(), jobData.remaining_delivery_minutes);
        ImGui::SameLine();
        {
          int total_minutes = static_cast<int>(jobData.remaining_delivery_minutes);
          int total_hours = total_minutes / 60;
          if (total_hours >= 24) {
            int days = total_hours / 24;
            int hours = total_hours % 24;
            int minutes = total_minutes % 60;
            ImGui::Text(loc.Get(m_locFormatDaysHoursMinutes).c_str(), days, hours, minutes);
          } else {
            int hours = total_hours;
            int minutes = total_minutes % 60;
            ImGui::Text(loc.Get(m_locFormatHoursMinutes).c_str(), hours, minutes);
          }
        }
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabNavigation).c_str())) {
      ImGui::Text(loc.Get(m_locLabelSpeedLimit).c_str(), navigationData.navigation_speed_limit * 3.6f);
      ImGui::Separator();
      ImGui::Text(loc.Get(m_locLabelNextWaypointDist).c_str(), navigationData.navigation_distance);

      ImGui::Text(loc.Get(m_locLabelNextWaypointTimeGame).c_str(), navigationData.navigation_time);
      ImGui::SameLine();
      {
        int total_seconds = static_cast<int>(navigationData.navigation_time);
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;
        ImGui::Text(loc.Get(m_locFormatHMS).c_str(), hours, minutes, seconds);
      }

      ImGui::Text(loc.Get(m_locLabelNextWaypointTimeReal).c_str(), navigationData.navigation_time_real_seconds);
      ImGui::SameLine();
      {
        int total_seconds = static_cast<int>(navigationData.navigation_time_real_seconds);
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        ImGui::Text(loc.Get(m_locFormatMS).c_str(), minutes, seconds);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabTruck).c_str())) {
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderConstants).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text(loc.Get(m_locLabelId).c_str(), truckConstants.id.c_str());
        ImGui::Text(loc.Get(m_locLabelBrand).c_str(), truckConstants.brand.c_str(), truckConstants.brand_id.c_str());
        ImGui::Text(loc.Get(m_locLabelName).c_str(), truckConstants.name.c_str());
        ImGui::Text(loc.Get(m_locLabelLicensePlate).c_str(), truckConstants.license_plate.c_str(), truckConstants.license_plate_country.c_str());

        ImGui::SeparatorText(loc.Get(m_locLabelEngineGearbox).c_str());
        ImGui::Text(loc.Get(m_locLabelRpmLimit).c_str(), truckConstants.rpm_limit);
        ImGui::Text(loc.Get(m_locLabelGears).c_str(), truckConstants.forward_gear_count, truckConstants.reverse_gear_count);
        ImGui::Text(loc.Get(m_locLabelRetarderSteps).c_str(), truckConstants.retarder_step_count);
        ImGui::Text(loc.Get(m_locLabelSelectorCount).c_str(), truckData.hshifter_selector.size());
        ImGui::Text(loc.Get(m_locLabelDifferentialRatio).c_str(), truckConstants.differential_ratio);
        ImGui::Text(loc.Get(m_locLabelShifterType).c_str(), gearboxConstants.shifter_type.c_str());

        if (ImGui::TreeNode(loc.Get(m_locLabelHshifterLayout).c_str())) {
          if (ImGui::BeginTable("hshifter_layout", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn(loc.Get(m_locLabelHshifterSlot).c_str());
            ImGui::TableSetupColumn(loc.Get(m_locLabelHshifterGear).c_str());
            ImGui::TableSetupColumn(loc.Get(m_locLabelHshifterHandlePos).c_str());
            ImGui::TableSetupColumn(loc.Get(m_locLabelHshifterSelectors).c_str());
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

        if (ImGui::TreeNode(loc.Get(m_locLabelGearRatios).c_str())) {
          if (ImGui::TreeNode(loc.Get(m_locLabelForward).c_str())) {
            for (size_t i = 0; i < truckConstants.gear_ratios_forward.size(); ++i) {
              ImGui::Text(loc.Get(m_locLabelGearX).c_str(), i + 1, truckConstants.gear_ratios_forward[i]);
            }
            ImGui::TreePop();
          }
          if (ImGui::TreeNode(loc.Get(m_locLabelReverse).c_str())) {
            for (size_t i = 0; i < truckConstants.gear_ratios_reverse.size(); ++i) {
              ImGui::Text(loc.Get(m_locLabelGearRx).c_str(), i + 1, truckConstants.gear_ratios_reverse[i]);
            }
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }

        ImGui::SeparatorText(loc.Get(m_locLabelCapacities).c_str());
        ImGui::Text(loc.Get(m_locLabelFuelCapacity).c_str(), truckConstants.fuel_capacity);
        ImGui::Text(loc.Get(m_locLabelAdblueCapacity).c_str(), truckConstants.adblue_capacity);

        ImGui::SeparatorText(loc.Get(m_locLabelWarningFactors).c_str());
        ImGui::Text(loc.Get(m_locLabelFuelWarning).c_str(), truckConstants.fuel_warning_factor * 100.0f);
        ImGui::Text(loc.Get(m_locLabelAdblueWarning).c_str(), truckConstants.adblue_warning_factor * 100.0f);
        ImGui::Text(loc.Get(m_locLabelAirPressureWarning).c_str(), truckConstants.air_pressure_warning);
        ImGui::Text(loc.Get(m_locLabelAirPressureEmergency).c_str(), truckConstants.air_pressure_emergency);
        ImGui::Text(loc.Get(m_locLabelOilPressureWarning).c_str(), truckConstants.oil_pressure_warning);
        ImGui::Text(loc.Get(m_locLabelWaterTempWarning).c_str(), truckConstants.water_temperature_warning);
        ImGui::Text(loc.Get(m_locLabelBatteryVoltageWarning).c_str(), truckConstants.battery_voltage_warning);
      }
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderLiveData).c_str())) {
        ImGui::SeparatorText(loc.Get(m_locLabelDashboardInfo).c_str());
        ImGui::Text(loc.Get(m_locLabelSpeed).c_str(), truckData.speed * 3.6f);
        ImGui::Text(loc.Get(m_locLabelEngineRpm).c_str(), truckData.engine_rpm);
        ImGui::Text(loc.Get(m_locLabelGear).c_str(), truckData.gear, truckData.displayed_gear);
        ImGui::Text(loc.Get(m_locLabelOdometer).c_str(), truckData.odometer);
        ImGui::Text(loc.Get(m_locLabelCruiseControl).c_str(),
                    truckData.cruise_control_speed > 0.0f ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.cruise_control_speed * 3.6f);
        ImGui::Text(loc.Get(m_locLabelFuel).c_str(),
                    truckData.fuel_amount,
                    truckData.fuel_average_consumption,
                    truckData.fuel_range);
        ImGui::Text(loc.Get(m_locLabelAdblue).c_str(), truckData.adblue_amount, truckData.adblue_average_consumption);
        ImGui::Text(loc.Get(m_locLabelOil).c_str(), truckData.oil_pressure, truckData.oil_temperature);
        ImGui::Text(loc.Get(m_locLabelWaterTemp).c_str(), truckData.water_temperature);
        ImGui::Text(loc.Get(m_locLabelBatteryVoltage).c_str(), truckData.battery_voltage);

        ImGui::SeparatorText(loc.Get(m_locLabelDashboardWarnings).c_str());
        ImGui::Text(loc.Get(m_locLabelFuelWarnState).c_str(), truckData.fuel_warning ? loc.Get(m_locGenericWarn).c_str() : loc.Get(m_locGenericOk).c_str());
        ImGui::Text(loc.Get(m_locLabelAdblueWarnState).c_str(), truckData.adblue_warning ? loc.Get(m_locGenericWarn).c_str() : loc.Get(m_locGenericOk).c_str());
        ImGui::Text(loc.Get(m_locLabelAirPressureWarnState).c_str(),
                    truckData.air_pressure_warning ? loc.Get(m_locGenericWarn).c_str() : (truckData.air_pressure_emergency ? loc.Get(m_locGenericEmergency).c_str() : loc.Get(m_locGenericOk).c_str()));
        ImGui::Text(loc.Get(m_locLabelOilPressureWarnState).c_str(), truckData.oil_pressure_warning ? loc.Get(m_locGenericWarn).c_str() : loc.Get(m_locGenericOk).c_str());
        ImGui::Text(loc.Get(m_locLabelWaterTempWarnState).c_str(), truckData.water_temperature_warning ? loc.Get(m_locGenericWarn).c_str() : loc.Get(m_locGenericOk).c_str());
        ImGui::Text(loc.Get(m_locLabelBatteryVoltageWarnState).c_str(), truckData.battery_voltage_warning ? loc.Get(m_locGenericWarn).c_str() : loc.Get(m_locGenericOk).c_str());

        ImGui::SeparatorText(loc.Get(m_locLabelSystemStates).c_str());
        ImGui::Text(loc.Get(m_locLabelElectricEnabled).c_str(), truckData.electric_enabled ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelEngineEnabled).c_str(), truckData.engine_enabled ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelDifferentialLock).c_str(), truckData.differential_lock ? loc.Get(m_locGenericEngaged).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelWipers).c_str(), truckData.wipers ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelTruckLiftAxle).c_str(),
                    truckData.lift_axle ? loc.Get(m_locGenericLifted).c_str() : loc.Get(m_locGenericDown).c_str(),
                    truckData.lift_axle_indicator ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelTrailerLiftAxle).c_str(),
                    truckData.trailer_lift_axle ? loc.Get(m_locGenericLifted).c_str() : loc.Get(m_locGenericDown).c_str(),
                    truckData.trailer_lift_axle_indicator ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());

        ImGui::SeparatorText(loc.Get(m_locLabelLights).c_str());
        ImGui::Text(loc.Get(m_locFormatBlinkerState).c_str(),
                    truckData.lblinker ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.light_lblinker ? "FLASH" : "_",
                    truckData.rblinker ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.light_rblinker ? "FLASH" : "_",
                    truckData.hazard_warning ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelLightStates).c_str(),
                    truckData.light_parking ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.light_low_beam ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.light_high_beam ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        auto aux_status_to_str = [&](uint32_t status) {
          if (status == 1) return loc.Get(m_locGenericDimmed).c_str();
          if (status == 2) return loc.Get(m_locGenericFull).c_str();
          return loc.Get(m_locGenericOff).c_str();
        };
        ImGui::Text(loc.Get(m_locLabelAuxLights).c_str(),
                    aux_status_to_str(truckData.light_aux_front),
                    aux_status_to_str(truckData.light_aux_roof),
                    truckData.light_beacon ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelBrakeReverseLights).c_str(),
                    truckData.light_brake ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str(),
                    truckData.light_reverse ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelDashboardBacklight).c_str(), truckData.dashboard_backlight);

        ImGui::SeparatorText(loc.Get(m_locLabelBrakes).c_str());
        ImGui::Text(loc.Get(m_locLabelAirPressure).c_str(), truckData.air_pressure);
        ImGui::Text(loc.Get(m_locLabelParkingBrake).c_str(), truckData.parking_brake ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelMotorBrake).c_str(), truckData.motor_brake ? loc.Get(m_locGenericOn).c_str() : loc.Get(m_locGenericOff).c_str());
        ImGui::Text(loc.Get(m_locLabelRetarderLevel).c_str(), truckData.retarder_level);
        ImGui::Text(loc.Get(m_locLabelBrakeTemp).c_str(), truckData.brake_temperature);

        ImGui::SeparatorText(loc.Get(m_locLabelHshifter).c_str());
        ImGui::Text(loc.Get(m_locLabelSlot).c_str(), truckData.hshifter_slot);
        std::string selectors_str;
        for (bool s : truckData.hshifter_selector) {
          selectors_str += s ? "1" : "0";
        }
        ImGui::Text(loc.Get(m_locLabelSelectors).c_str(), selectors_str.c_str());
      }
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderPhysics).c_str())) {
        DisplayFVector(loc.Get(m_locLabelLinearVelocity).c_str(), truckData.local_linear_velocity);
        DisplayFVector(loc.Get(m_locLabelAngularVelocity).c_str(), truckData.local_angular_velocity);
        DisplayFVector(loc.Get(m_locLabelLinearAccel).c_str(), truckData.local_linear_acceleration);
        DisplayFVector(loc.Get(m_locLabelAngularAccel).c_str(), truckData.local_angular_acceleration);
        ImGui::Separator();
        DisplayFVector(loc.Get(m_locLabelCabinAngVel).c_str(), truckData.cabin_angular_velocity);
        DisplayFVector(loc.Get(m_locLabelCabinAngAccel).c_str(), truckData.cabin_angular_acceleration);
      }
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderWheels).c_str())) {
        for (size_t i = 0; i < truckData.wheels.size(); ++i) {
          if (i < truckConstants.wheels.size()) {
            const auto& wheel_data = truckData.wheels[i];
            const auto& wheel_const = truckConstants.wheels[i];
            const std::string& wheel_node_format = loc.Get(m_locLabelWheelX);
            std::string wheel_node_id = fmt::format(fmt::runtime(wheel_node_format), i);
            if (ImGui::TreeNode(wheel_node_id.c_str())) {
              const auto& substances = commonData.substances;
              if (wheel_data.substance < substances.size()) {
                ImGui::Text(loc.Get(m_locLabelSubstance).c_str(), substances[wheel_data.substance].c_str(), wheel_data.substance);
              } else {
                ImGui::Text(loc.Get(m_locLabelSubstanceUnknown).c_str(), wheel_data.substance);
              }

              ImGui::Text(loc.Get(m_locLabelOnGround).c_str(), wheel_data.on_ground ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
              ImGui::Text(loc.Get(m_locLabelSuspDeflection).c_str(), wheel_data.suspension_deflection);
              ImGui::Text(loc.Get(m_locLabelWheelVelocity).c_str(), wheel_data.angular_velocity);
              ImGui::Text(loc.Get(m_locLabelSteering).c_str(), wheel_data.steering);
              ImGui::Text(loc.Get(m_locLabelRotation).c_str(), wheel_data.rotation);
              ImGui::Text(loc.Get(m_locLabelLift).c_str(), wheel_data.lift, wheel_data.lift_offset);
              ImGui::Separator();
              ImGui::Text(loc.Get(m_locLabelSteerable).c_str(), wheel_const.steerable ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
              ImGui::Text(loc.Get(m_locLabelPowered).c_str(), wheel_const.powered ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
              ImGui::Text(loc.Get(m_locLabelLiftable).c_str(), wheel_const.liftable ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
              ImGui::Text(loc.Get(m_locLabelSimulated).c_str(), wheel_const.simulated ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
              ImGui::Text(loc.Get(m_locLabelRadius).c_str(), wheel_const.radius);
              DisplayFVector(loc.Get(m_locLabelPosition).c_str(), wheel_const.position);
              ImGui::TreePop();
            }
          }
        }
      }

      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderDamage).c_str())) {
        ImGui::Text(loc.Get(m_locLabelEngineDamage).c_str(), truckData.wear_engine * 100.0f);
        ImGui::Text(loc.Get(m_locLabelTransmissionDamage).c_str(), truckData.wear_transmission * 100.0f);
        ImGui::Text(loc.Get(m_locLabelCabinDamage).c_str(), truckData.wear_cabin * 100.0f);
        ImGui::Text(loc.Get(m_locLabelChassisDamage).c_str(), truckData.wear_chassis * 100.0f);
        ImGui::Text(loc.Get(m_locLabelWheelsDamage).c_str(), truckData.wear_wheels * 100.0f);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabPositioning).c_str())) {
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderTruckPositioning).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SeparatorText(loc.Get(m_locLabelWorldSpace).c_str());
        DisplayDPlacement(loc.Get(m_locLabelWorldPlacement).c_str(), truckData.world_placement, loc.Get(m_locFormatPlacementPos), loc.Get(m_locFormatPlacementOri));

        ImGui::SeparatorText(loc.Get(m_locLabelComponentOffsets).c_str());
        DisplayFPlacement(loc.Get(m_locLabelCabinOffset).c_str(), truckData.cabin_offset, loc.Get(m_locFormatPlacementPos), loc.Get(m_locFormatPlacementOri));
        DisplayFPlacement(loc.Get(m_locLabelHeadOffset).c_str(), truckData.head_offset, loc.Get(m_locFormatPlacementPos), loc.Get(m_locFormatPlacementOri));

        ImGui::SeparatorText(loc.Get(m_locLabelComponentBasePos).c_str());
        DisplayFVector(loc.Get(m_locLabelCabinPos).c_str(), truckConstants.cabin_position);
        DisplayFVector(loc.Get(m_locLabelHeadPos).c_str(), truckConstants.head_position);
        DisplayFVector(loc.Get(m_locLabelHookPos).c_str(), truckConstants.hook_position);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabTrailers).c_str())) {
      for (size_t i = 0; i < trailers.size(); ++i) {
        const auto& trailer = trailers[i];
        // Skip rendering trailers that are not connected and have no configuration data.
        if (!trailer.data.connected && trailer.constants.id.empty()) continue;

        const std::string& trailer_node_format = loc.Get(m_locLabelTrailerX);
        std::string trailer_node_id =
            fmt::format(fmt::runtime(trailer_node_format), i, trailer.constants.id.empty() ? loc.Get(m_locLabelTrailerNa).c_str() : trailer.constants.id.c_str());
        if (ImGui::TreeNode(trailer_node_id.c_str())) {
          ImGui::Text(loc.Get(m_locLabelConnected).c_str(), trailer.data.connected ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());

          // --- General Info ---
          std::string general_label = loc.Get(m_locLabelGeneral) + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(general_label.c_str())) {
            ImGui::Text(loc.Get(m_locLabelId).c_str(), trailer.constants.id.c_str());
            ImGui::Text(loc.Get(m_locLabelName).c_str(), trailer.constants.name.c_str());
            ImGui::Text(loc.Get(m_locLabelTrailerBrand).c_str(), trailer.constants.brand.c_str(), trailer.constants.brand_id.c_str());
            ImGui::Text(loc.Get(m_locLabelTrailerLicensePlate).c_str(),
                        trailer.constants.license_plate.c_str(),
                        trailer.constants.license_plate_country.c_str(),
                        trailer.constants.license_plate_country_id.c_str());
            ImGui::Text(loc.Get(m_locLabelBodyType).c_str(), trailer.constants.body_type.c_str());
            ImGui::Text(loc.Get(m_locLabelChainType).c_str(), trailer.constants.chain_type.c_str());
            ImGui::Text(loc.Get(m_locLabelCargoAccessoryId).c_str(), trailer.constants.cargo_accessory_id.c_str());
            ImGui::TreePop();
          }

          // --- Physics & Position ---
          std::string physics_pos_label = loc.Get(m_locLabelPhysicsPos) + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(physics_pos_label.c_str())) {
            DisplayFVector(loc.Get(m_locLabelTrailerHookPos).c_str(), trailer.constants.hook_position);
            ImGui::Text(loc.Get(m_locLabelTrailerWorldPos).c_str(),
                        trailer.data.world_placement.position.x,
                        trailer.data.world_placement.position.y,
                        trailer.data.world_placement.position.z);
            ImGui::Separator();
            DisplayFVector(loc.Get(m_locLabelLinearVelocity).c_str(), trailer.data.local_linear_velocity);
            DisplayFVector(loc.Get(m_locLabelAngularVelocity).c_str(), trailer.data.local_angular_velocity);
            DisplayFVector(loc.Get(m_locLabelLinearAccel).c_str(), trailer.data.local_linear_acceleration);
            DisplayFVector(loc.Get(m_locLabelAngularAccel).c_str(), trailer.data.local_angular_acceleration);
            ImGui::TreePop();
          }

          // --- Damage ---
          std::string damage_label = loc.Get(m_locHeaderDamage) + "##Trailer" + std::to_string(i);
          if (ImGui::TreeNode(damage_label.c_str())) {
            ImGui::Text(loc.Get(m_locLabelTrailerDamageBody).c_str(), trailer.data.wear_body * 100.0f);
            ImGui::Text(loc.Get(m_locLabelTrailerDamageChassis).c_str(), trailer.data.wear_chassis * 100.0f);
            ImGui::Text(loc.Get(m_locLabelTrailerDamageWheels).c_str(), trailer.data.wear_wheels * 100.0f);
            ImGui::Text(loc.Get(m_locLabelTrailerDamageCargo).c_str(), trailer.data.cargo_damage * 100.0f);
            ImGui::TreePop();
          }

          // --- Wheels ---
          std::string wheels_header_str = loc.Get(m_locLabelWheelsCount);
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
              std::string wheel_node_str = loc.Get(m_locLabelWheelX);
              size_t pos = wheel_node_str.find("{}");
              if (pos != std::string::npos) {
                wheel_node_str.replace(pos, 2, std::to_string(j));
              }
              wheel_node_str += "##Trailer" + std::to_string(i);

              if (ImGui::TreeNode(wheel_node_str.c_str())) {
                // Live Data
                ImGui::Text(loc.Get(m_locLabelOnGround).c_str(), wheel_data.on_ground ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
                ImGui::Text(loc.Get(m_locLabelWheelSuspDeflection).c_str(), wheel_data.suspension_deflection);
                ImGui::Text(loc.Get(m_locLabelWheelAngularVelocity).c_str(), wheel_data.angular_velocity);
                ImGui::Text(loc.Get(m_locLabelSteering).c_str(), wheel_data.steering);
                ImGui::Text(loc.Get(m_locLabelRotation).c_str(), wheel_data.rotation);
                ImGui::Text(loc.Get(m_locLabelWheelLift).c_str(), wheel_data.lift);
                ImGui::Text(loc.Get(m_locLabelWheelLiftOffset).c_str(), wheel_data.lift_offset);
                if (wheel_data.substance < commonData.substances.size()) {
                  ImGui::Text(loc.Get(m_locLabelSubstance).c_str(), commonData.substances[wheel_data.substance].c_str(), wheel_data.substance);
                } else {
                  ImGui::Text(loc.Get(m_locLabelSubstanceUnknown).c_str(), wheel_data.substance);
                }
                ImGui::Separator();
                // Constant Data
                ImGui::Text(loc.Get(m_locLabelRadius).c_str(), wheel_const.radius);
                ImGui::Text(loc.Get(m_locLabelSteerable).c_str(), wheel_const.steerable ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
                ImGui::Text(loc.Get(m_locLabelPowered).c_str(), wheel_const.powered ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
                ImGui::Text(loc.Get(m_locLabelLiftable).c_str(), wheel_const.liftable ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
                ImGui::Text(loc.Get(m_locLabelSimulated).c_str(), wheel_const.simulated ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
                DisplayFVector(loc.Get(m_locLabelPosition).c_str(), wheel_const.position);
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

    if (ImGui::BeginTabItem(loc.Get(m_locTabControlsEvents).c_str())) {
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderControls).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(loc.Get(m_locLabelUserInput).c_str());
        ImGui::Text(loc.Get(m_locLabelInputSteering).c_str(), controls.userInput.steering);
        ImGui::Text(loc.Get(m_locLabelInputThrottle).c_str(), controls.userInput.throttle);
        ImGui::Text(loc.Get(m_locLabelInputBrake).c_str(), controls.userInput.brake);
        ImGui::Text(loc.Get(m_locLabelInputClutch).c_str(), controls.userInput.clutch);
        ImGui::Separator();
        ImGui::TextUnformatted(loc.Get(m_locLabelEffectiveInput).c_str());
        ImGui::Text(loc.Get(m_locLabelInputSteering).c_str(), controls.effectiveInput.steering);
        ImGui::Text(loc.Get(m_locLabelInputThrottle).c_str(), controls.effectiveInput.throttle);
        ImGui::Text(loc.Get(m_locLabelInputBrake).c_str(), controls.effectiveInput.brake);
        ImGui::Text(loc.Get(m_locLabelInputClutch).c_str(), controls.effectiveInput.clutch);
      }
      if (ImGui::CollapsingHeader(loc.Get(m_locHeaderEvents).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(loc.Get(m_locLabelSpecialEvents).c_str());
        ImGui::Text(loc.Get(m_locLabelOnJob).c_str(), jobData.on_job ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelJobCancelled).c_str(), specialEvents.job_cancelled ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelJobDelivered).c_str(), specialEvents.job_delivered ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelFined).c_str(), specialEvents.fined ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelTollgate).c_str(), specialEvents.tollgate ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelFerry).c_str(), specialEvents.ferry ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Text(loc.Get(m_locLabelTrain).c_str(), specialEvents.train ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        ImGui::Separator();
        ImGui::TextUnformatted(loc.Get(m_locLabelLastGameplayEvent).c_str());
        const std::string& lastEventId = m_lastGameplayEventId;
        if (lastEventId.empty()) {
          ImGui::TextUnformatted(loc.Get(m_locLabelNoEventYet).c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_job_delivered) {
          const auto& data = gameplayEvents.job_delivered;
          ImGui::Text(loc.Get(m_locLabelEventJobDelivered).c_str(), data.revenue, data.earned_xp, data.cargo_damage * 100.0f);
          ImGui::Text(loc.Get(m_locLabelEventJobDeliveredDetails).c_str(), data.distance_km, data.delivery_time);
          ImGui::Text(loc.Get(m_locLabelEventJobDeliveredFlags).c_str(),
                      data.auto_park_used ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str(),
                      data.auto_load_used ? loc.Get(m_locGenericYes).c_str() : loc.Get(m_locGenericNo).c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_job_cancelled) {
          const auto& data = gameplayEvents.job_cancelled;
          ImGui::Text(loc.Get(m_locLabelEventJobCancelled).c_str(), data.penalty);
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_fined) {
          const auto& data = gameplayEvents.player_fined;
          ImGui::Text(loc.Get(m_locLabelEventFined).c_str(), data.fine_amount, data.fine_offence.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_tollgate_paid) {
          const auto& data = gameplayEvents.tollgate_paid;
          ImGui::Text(loc.Get(m_locLabelEventTollgate).c_str(), data.pay_amount);
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_ferry) {
          const auto& data = gameplayEvents.ferry_used;
          ImGui::Text(loc.Get(m_locLabelEventFerry).c_str(), data.pay_amount);
          ImGui::Text(loc.Get(m_locLabelEventFerryRoute).c_str(), data.source_name.c_str(), data.source_id.c_str());
          ImGui::Text(loc.Get(m_locLabelEventFerryRouteTo).c_str(), data.target_name.c_str(), data.target_id.c_str());
        } else if (lastEventId == SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_train) {
          const auto& data = gameplayEvents.train_used;
          ImGui::Text(loc.Get(m_locLabelEventTrain).c_str(), data.pay_amount);
          ImGui::Text(loc.Get(m_locLabelEventTrainRoute).c_str(), data.source_name.c_str(), data.source_id.c_str());
          ImGui::Text(loc.Get(m_locLabelEventTrainRouteTo).c_str(), data.target_name.c_str(), data.target_id.c_str());
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