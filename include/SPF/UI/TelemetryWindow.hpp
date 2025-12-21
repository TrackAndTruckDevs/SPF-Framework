#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Utils/Signal.hpp"
#include <string>
#include <vector>

SPF_NS_BEGIN

namespace Modules {
class ITelemetryService;
}



namespace UI {
class TelemetryWindow : public BaseWindow {
 public:
  TelemetryWindow(const std::string& componentName, const std::string& windowId, Modules::ITelemetryService& telemetryService);

 protected:
  const char* GetWindowTitle() const override;
  void RenderContent() override;

 private:
  // Event Handlers (Slots)
  void OnGameStateUpdate(const Telemetry::SCS::GameState& data);
  void OnTimestampsUpdate(const Telemetry::SCS::Timestamps& data);
  void OnCommonDataUpdate(const Telemetry::SCS::CommonData& data);
  void OnTruckConstantsUpdate(const Telemetry::SCS::TruckConstants& data);
  void OnTruckDataUpdate(const Telemetry::SCS::TruckData& data);
  void OnTrailersUpdate(const std::vector<Telemetry::SCS::Trailer>& data);
  void OnJobConstantsUpdate(const Telemetry::SCS::JobConstants& data);
  void OnJobDataUpdate(const Telemetry::SCS::JobData& data);
  void OnNavigationDataUpdate(const Telemetry::SCS::NavigationData& data);
  void OnControlsUpdate(const Telemetry::SCS::Controls& data);
  void OnSpecialEventsUpdate(const Telemetry::SCS::SpecialEvents& data);
  void OnGameplayEventUpdate(const char* event_id, const Telemetry::SCS::GameplayEvents& data);
  void OnGearboxConstantsUpdate(const Telemetry::SCS::GearboxConstants& data);

 private:
  Modules::ITelemetryService& m_telemetryService;

  // Signal Sinks
  Utils::Sink<void(const Telemetry::SCS::GameState&)> m_gameStateSink;
  Utils::Sink<void(const Telemetry::SCS::Timestamps&)> m_timestampsSink;
  Utils::Sink<void(const Telemetry::SCS::CommonData&)> m_commonDataSink;
  Utils::Sink<void(const Telemetry::SCS::TruckConstants&)> m_truckConstantsSink;
  Utils::Sink<void(const Telemetry::SCS::TruckData&)> m_truckDataSink;
  Utils::Sink<void(const std::vector<Telemetry::SCS::Trailer>&)> m_trailersSink;
  Utils::Sink<void(const Telemetry::SCS::JobConstants&)> m_jobConstantsSink;
  Utils::Sink<void(const Telemetry::SCS::JobData&)> m_jobDataSink;
  Utils::Sink<void(const Telemetry::SCS::NavigationData&)> m_navigationDataSink;
  Utils::Sink<void(const Telemetry::SCS::Controls&)> m_controlsSink;
  Utils::Sink<void(const Telemetry::SCS::SpecialEvents&)> m_specialEventsSink;
  Utils::Sink<void(const char*, const Telemetry::SCS::GameplayEvents&)> m_gameplayEventsSink;
  Utils::Sink<void(const Telemetry::SCS::GearboxConstants&)> m_gearboxConstantsSink;

  // Data Cache
  Telemetry::SCS::GameState m_gameState;
  Telemetry::SCS::Timestamps m_timestamps;
  Telemetry::SCS::CommonData m_commonData;
  Telemetry::SCS::TruckConstants m_truckConstants;
  Telemetry::SCS::TruckData m_truckData;
  std::vector<Telemetry::SCS::Trailer> m_trailers;
  Telemetry::SCS::JobConstants m_jobConstants;
  Telemetry::SCS::JobData m_jobData;
  Telemetry::SCS::NavigationData m_navigationData;
  Telemetry::SCS::Controls m_controls;
  Telemetry::SCS::SpecialEvents m_specialEvents;
  Telemetry::SCS::GameplayEvents m_gameplayEvents;
  Telemetry::SCS::GearboxConstants m_gearboxConstants;
  std::string m_lastGameplayEventId;
  
  // Localization keys
  std::string m_locTabGame;
  std::string m_locTabJob;
  std::string m_locTabNavigation;
  std::string m_locTabTruck;
  std::string m_locTabPositioning;
  std::string m_locTabTrailers;
  std::string m_locTabControlsEvents;

  std::string m_locHeaderGameState;
  std::string m_locHeaderConstants;
  std::string m_locHeaderLiveData;
  std::string m_locHeaderPhysics;
  std::string m_locHeaderWheels;
  std::string m_locHeaderDamage;
  std::string m_locHeaderTruckPositioning;
  std::string m_locHeaderControls;
  std::string m_locHeaderEvents;

  std::string m_locLabelGameTime;
  std::string m_locLabelNextRestStop;
  std::string m_locLabelNextRestStopReal;
  std::string m_locLabelNextRestStopTime;
  std::string m_locLabelPaused;
  std::string m_locLabelGameId;
  std::string m_locLabelLocalScale;
  std::string m_locLabelMultiplayerTimeOffset;
  std::string m_locLabelScsGameVersion;
  std::string m_locLabelTelemetryPluginVersion;
  std::string m_locLabelTelemetryGameVersion;
  std::string m_locLabelGameName;
  std::string m_locLabelSubstances;
  std::string m_locLabelSubstancesNotReceived;
  std::string m_locLabelSimulationTime;
  std::string m_locLabelRenderTime;
  std::string m_locLabelPausedSimulationTime;
  std::string m_locLabelNoActiveJob;
  std::string m_locLabelContract;
  std::string m_locLabelMarket;
  std::string m_locLabelIncome;
  std::string m_locLabelPlannedDistance;
  std::string m_locLabelCargo;
  std::string m_locLabelCargoInfo;
  std::string m_locLabelMass;
  std::string m_locLabelDamage;
  std::string m_locLabelLoaded;
  std::string m_locLabelSpecialJob;
  std::string m_locLabelRoute;
  std::string m_locLabelSource;
  std::string m_locLabelDestination;
  std::string m_locLabelTime;
  std::string m_locLabelDeliveryDeadline;
  std::string m_locLabelRemainingGameTime;
  std::string m_locLabelSpeedLimit;
  std::string m_locLabelNextWaypointDist;
  std::string m_locLabelNextWaypointTimeGame;
  std::string m_locLabelNextWaypointTimeReal;
  std::string m_locLabelId;
  std::string m_locLabelBrand;
  std::string m_locLabelName;
  std::string m_locLabelLicensePlate;
  std::string m_locLabelEngineGearbox;
  std::string m_locLabelRpmLimit;
  std::string m_locLabelGears;
  std::string m_locLabelRetarderSteps;
  std::string m_locLabelSelectorCount;
  std::string m_locLabelDifferentialRatio;
  std::string m_locLabelShifterType;
  std::string m_locLabelHshifterLayout;
  std::string m_locLabelHshifterSlot;
  std::string m_locLabelHshifterGear;
  std::string m_locLabelHshifterHandlePos;
  std::string m_locLabelHshifterSelectors;
  std::string m_locLabelGearRatios;
  std::string m_locLabelForward;
  std::string m_locLabelReverse;
  std::string m_locLabelGearX;
  std::string m_locLabelGearRx;
  std::string m_locLabelCapacities;
  std::string m_locLabelFuelCapacity;
  std::string m_locLabelAdblueCapacity;
  std::string m_locLabelWarningFactors;
  std::string m_locLabelFuelWarning;
  std::string m_locLabelAdblueWarning;
  std::string m_locLabelAirPressureWarning;
  std::string m_locLabelAirPressureEmergency;
  std::string m_locLabelOilPressureWarning;
  std::string m_locLabelWaterTempWarning;
  std::string m_locLabelBatteryVoltageWarning;
  std::string m_locLabelDashboardInfo;
  std::string m_locLabelSpeed;
  std::string m_locLabelEngineRpm;
  std::string m_locLabelGear;
  std::string m_locLabelOdometer;
  std::string m_locLabelCruiseControl;
  std::string m_locLabelFuel;
  std::string m_locLabelAdblue;
  std::string m_locLabelOil;
  std::string m_locLabelWaterTemp;
  std::string m_locLabelBatteryVoltage;
  std::string m_locLabelDashboardWarnings;
  std::string m_locLabelFuelWarnState;
  std::string m_locLabelAdblueWarnState;
  std::string m_locLabelAirPressureWarnState;
  std::string m_locLabelOilPressureWarnState;
  std::string m_locLabelWaterTempWarnState;
  std::string m_locLabelBatteryVoltageWarnState;
  std::string m_locLabelSystemStates;
  std::string m_locLabelElectricEnabled;
  std::string m_locLabelEngineEnabled;
  std::string m_locLabelDifferentialLock;
  std::string m_locLabelWipers;
  std::string m_locLabelTruckLiftAxle;
  std::string m_locLabelTrailerLiftAxle;
  std::string m_locLabelLights;
  std::string m_locLabelBlinkers;
  std::string m_locLabelLightStates;
  std::string m_locLabelAuxLights;
  std::string m_locLabelBrakeReverseLights;
  std::string m_locLabelDashboardBacklight;
  std::string m_locLabelBrakes;
  std::string m_locLabelAirPressure;
  std::string m_locLabelParkingBrake;
  std::string m_locLabelMotorBrake;
  std::string m_locLabelRetarderLevel;
  std::string m_locLabelBrakeTemp;
  std::string m_locLabelHshifter;
  std::string m_locLabelSlot;
  std::string m_locLabelSelectors;
  std::string m_locLabelLinearVelocity;
  std::string m_locLabelAngularVelocity;
  std::string m_locLabelLinearAccel;
  std::string m_locLabelAngularAccel;
  std::string m_locLabelCabinAngVel;
  std::string m_locLabelCabinAngAccel;
  std::string m_locLabelWheelX;
  std::string m_locLabelSubstance;
  std::string m_locLabelSubstanceUnknown;
  std::string m_locLabelOnGround;
  std::string m_locLabelSuspDeflection;
  std::string m_locLabelWheelVelocity;
  std::string m_locLabelSteering;
  std::string m_locLabelRotation;
  std::string m_locLabelLift;
  std::string m_locLabelSteerable;
  std::string m_locLabelPowered;
  std::string m_locLabelLiftable;
  std::string m_locLabelSimulated;
  std::string m_locLabelRadius;
  std::string m_locLabelPosition;
  std::string m_locLabelEngineDamage;
  std::string m_locLabelTransmissionDamage;
  std::string m_locLabelCabinDamage;
  std::string m_locLabelChassisDamage;
  std::string m_locLabelWheelsDamage;
  std::string m_locLabelWorldSpace;
  std::string m_locLabelWorldPlacement;
  std::string m_locLabelComponentOffsets;
  std::string m_locLabelCabinOffset;
  std::string m_locLabelHeadOffset;
  std::string m_locLabelComponentBasePos;
  std::string m_locLabelCabinPos;
  std::string m_locLabelHeadPos;
  std::string m_locLabelHookPos;
  std::string m_locLabelTrailerX;
  std::string m_locLabelTrailerNa;
  std::string m_locLabelConnected;
  std::string m_locLabelGeneral;
  std::string m_locLabelTrailerBrand;
  std::string m_locLabelTrailerLicensePlate;
  std::string m_locLabelBodyType;
  std::string m_locLabelChainType;
  std::string m_locLabelCargoAccessoryId;
  std::string m_locLabelPhysicsPos;
  std::string m_locLabelTrailerHookPos;
  std::string m_locLabelTrailerWorldPos;
  std::string m_locLabelTrailerDamageBody;
  std::string m_locLabelTrailerDamageChassis;
  std::string m_locLabelTrailerDamageWheels;
  std::string m_locLabelTrailerDamageCargo;
  std::string m_locLabelWheelsCount;
  std::string m_locLabelWheelSuspDeflection;
  std::string m_locLabelWheelAngularVelocity;
  std::string m_locLabelWheelLift;
  std::string m_locLabelWheelLiftOffset;
  std::string m_locLabelUserInput;
  std::string m_locLabelEffectiveInput;
  std::string m_locLabelInputSteering;
  std::string m_locLabelInputThrottle;
  std::string m_locLabelInputBrake;
  std::string m_locLabelInputClutch;
  std::string m_locLabelSpecialEvents;
  std::string m_locLabelOnJob;
  std::string m_locLabelJobCancelled;
  std::string m_locLabelJobDelivered;
  std::string m_locLabelFined;
  std::string m_locLabelTollgate;
  std::string m_locLabelFerry;
  std::string m_locLabelTrain;
  std::string m_locLabelLastGameplayEvent;
  std::string m_locLabelNoEventYet;
  std::string m_locLabelEventJobDelivered;
  std::string m_locLabelEventJobDeliveredDetails;
  std::string m_locLabelEventJobDeliveredFlags;
  std::string m_locLabelEventJobCancelled;
  std::string m_locLabelEventFined;
  std::string m_locLabelEventTollgate;
  std::string m_locLabelEventFerry;
  std::string m_locLabelEventFerryRoute;
  std::string m_locLabelEventFerryRouteTo;
  std::string m_locLabelEventTrain;
  std::string m_locLabelEventTrainRoute;
  std::string m_locLabelEventTrainRouteTo;

  std::vector<std::string> m_locDaysOfWeek;
  std::string m_locFormatDayHourMinute;
  std::string m_locFormatDaysHoursMinutes;
  std::string m_locFormatHoursMinutes;
  std::string m_locFormatRealTimeMinutes;
  std::string m_locFormatRealTimeHoursMinutes;
  std::string m_locFormatNextRestStopTime;
  std::string m_locFormatKmH;
  std::string m_locFormatMeters;
  std::string m_locFormatGameTimeSeconds;
  std::string m_locFormatHMS;
  std::string m_locFormatRealTimeSeconds;
  std::string m_locFormatMS;
  std::string m_locFormatGearsFwdRev;
  std::string m_locFormatLiters;
  std::string m_locFormatPercent;
  std::string m_locFormatPressurePsi;
  std::string m_locFormatTempCelsius;
  std::string m_locFormatVoltageV;
  std::string m_locFormatSpeedKmH;
  std::string m_locFormatCruiseControlSpeed;
  std::string m_locFormatFuelConsumption;
  std::string m_locFormatFuelRange;
  std::string m_locFormatAdblueConsumption;
  std::string m_locFormatOilPressureTemp;
  std::string m_locFormatTempCelsiusF;
  std::string m_locFormatVoltageVF;
  std::string m_locFormatBlinkerState;
  std::string m_locFormatDashboardBacklight;
  std::string m_locFormatDamagePercent;
  std::string m_locFormatVector;
  std::string m_locFormatPlacementPos;
  std::string m_locFormatPlacementOri;
  std::string m_locFormatTrailerWorldPos;
  std::string m_locFormatDamagePercent2f;

  std::string m_locGenericYes;
  std::string m_locGenericNo;
  std::string m_locGenericOn;
  std::string m_locGenericOff;
  std::string m_locGenericWarn;
  std::string m_locGenericEmergency;
  std::string m_locGenericOk;
  std::string m_locGenericEngaged;
  std::string m_locGenericLifted;
  std::string m_locGenericDown;
  std::string m_locGenericDimmed;
  std::string m_locGenericFull;
};
}  // namespace UI

SPF_NS_END