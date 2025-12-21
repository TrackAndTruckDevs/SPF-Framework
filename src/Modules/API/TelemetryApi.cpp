#include "SPF/Modules/API/TelemetryApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/TelemetryHandle.hpp"
#include "SPF/Modules/ITelemetryService.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include <algorithm>
#include <cstring>

SPF_NS_BEGIN
namespace Modules::API {

using namespace Telemetry::SCS;

// Helper to convert Game enum to string
const char* GameToString(SPF::Game game) {
  switch (game) {
    case SPF::Game::ETS2:
      return "ETS2";
    case SPF::Game::ATS:
      return "ATS";
    case SPF::Game::Unknown:
      return "Unknown";
    default:
      return "Invalid";
  }
}

SPF_Telemetry_Handle* TelemetryApi::T_GetContext(const char* pluginName) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto handle = std::make_unique<Handles::TelemetryHandle>(pluginName);
    return reinterpret_cast<SPF_Telemetry_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

void TelemetryApi::T_GetGameState(SPF_Telemetry_Handle* handle, SPF_GameState* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetGameState();

    strcpy_s(out_data->game_id, SPF_TELEMETRY_ID_MAX_SIZE, GameToString(cpp_data.game_id));
    strcpy_s(out_data->game_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.game_name.c_str());
    
    out_data->scs_game_version_major = cpp_data.scs_game_version_major;
    out_data->scs_game_version_minor = cpp_data.scs_game_version_minor;
    
    out_data->telemetry_plugin_version_major = cpp_data.telemetry_plugin_version_major;
    out_data->telemetry_plugin_version_minor = cpp_data.telemetry_plugin_version_minor;

    out_data->telemetry_game_version_major = cpp_data.telemetry_game_version_major;
    out_data->telemetry_game_version_minor = cpp_data.telemetry_game_version_minor;

    out_data->paused = cpp_data.paused;
    out_data->scale = cpp_data.scale;
    out_data->multiplayer_time_offset = cpp_data.multiplayer_time_offset;
}

void TelemetryApi::T_GetTimestamps(SPF_Telemetry_Handle* handle, SPF_Timestamps* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetTimestamps();
    out_data->simulation = cpp_data.simulation;
    out_data->render = cpp_data.render;
    out_data->paused_simulation = cpp_data.paused_simulation;
}

void TelemetryApi::T_GetCommonData(SPF_Telemetry_Handle* handle, SPF_CommonData* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetCommonData();
    out_data->game_time = cpp_data.game_time;
    out_data->next_rest_stop = cpp_data.next_rest_stop;
    out_data->next_rest_stop_time.DayOfWeek = cpp_data.next_rest_stop_time.DayOfWeek;
    out_data->next_rest_stop_time.Hour = cpp_data.next_rest_stop_time.Hour;
    out_data->next_rest_stop_time.Minute = cpp_data.next_rest_stop_time.Minute;
    out_data->next_rest_stop_real_minutes = cpp_data.next_rest_stop_real_minutes;

    out_data->substance_count = static_cast<uint32_t>(std::min<size_t>(cpp_data.substances.size(), SPF_TELEMETRY_SUBSTANCE_MAX_COUNT));
    for (uint32_t i = 0; i < out_data->substance_count; ++i) {
        strcpy_s(out_data->substances[i], SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.substances[i].c_str());
    }
}

void TelemetryApi::T_GetTruckConstants(SPF_Telemetry_Handle* handle, SPF_TruckConstants* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetTruckConstants();

    strcpy_s(out_data->brand_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.brand_id.c_str());
    strcpy_s(out_data->brand, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.brand.c_str());
    strcpy_s(out_data->id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.id.c_str());
    strcpy_s(out_data->name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.name.c_str());
    strcpy_s(out_data->license_plate, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate.c_str());
    strcpy_s(out_data->license_plate_country_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate_country_id.c_str());
    strcpy_s(out_data->license_plate_country, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.license_plate_country.c_str());

    out_data->fuel_capacity = cpp_data.fuel_capacity;
    out_data->fuel_warning_factor = cpp_data.fuel_warning_factor;
    out_data->adblue_capacity = cpp_data.adblue_capacity;
    out_data->adblue_warning_factor = cpp_data.adblue_warning_factor;
    out_data->air_pressure_warning = cpp_data.air_pressure_warning;
    out_data->air_pressure_emergency = cpp_data.air_pressure_emergency;
    out_data->oil_pressure_warning = cpp_data.oil_pressure_warning;
    out_data->water_temperature_warning = cpp_data.water_temperature_warning;
    out_data->battery_voltage_warning = cpp_data.battery_voltage_warning;
    out_data->rpm_limit = cpp_data.rpm_limit;
    out_data->forward_gear_count = cpp_data.forward_gear_count;
    out_data->reverse_gear_count = cpp_data.reverse_gear_count;
    out_data->retarder_step_count = cpp_data.retarder_step_count;
    out_data->selector_count = cpp_data.selector_count;
    out_data->differential_ratio = cpp_data.differential_ratio;

    out_data->cabin_position = {cpp_data.cabin_position.x, cpp_data.cabin_position.y, cpp_data.cabin_position.z};
    out_data->head_position = {cpp_data.head_position.x, cpp_data.head_position.y, cpp_data.head_position.z};
    out_data->hook_position = {cpp_data.hook_position.x, cpp_data.hook_position.y, cpp_data.hook_position.z};

    out_data->wheel_count = std::min<uint32_t>(cpp_data.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
    for (uint32_t i = 0; i < out_data->wheel_count; ++i) {
        out_data->wheels[i].simulated = cpp_data.wheels[i].simulated;
        out_data->wheels[i].powered = cpp_data.wheels[i].powered;
        out_data->wheels[i].steerable = cpp_data.wheels[i].steerable;
        out_data->wheels[i].liftable = cpp_data.wheels[i].liftable;
        out_data->wheels[i].radius = cpp_data.wheels[i].radius;
        out_data->wheels[i].position = {cpp_data.wheels[i].position.x, cpp_data.wheels[i].position.y, cpp_data.wheels[i].position.z};
    }

    uint32_t forward_gears_to_copy = std::min<uint32_t>(cpp_data.forward_gear_count, SPF_TELEMETRY_GEAR_MAX_COUNT);
    memcpy(out_data->gear_ratios_forward, cpp_data.gear_ratios_forward.data(), forward_gears_to_copy * sizeof(float));

    uint32_t reverse_gears_to_copy = std::min<uint32_t>(cpp_data.reverse_gear_count, SPF_TELEMETRY_GEAR_MAX_COUNT);
    memcpy(out_data->gear_ratios_reverse, cpp_data.gear_ratios_reverse.data(), reverse_gears_to_copy * sizeof(float));
}

void TelemetryApi::T_GetTruckData(SPF_Telemetry_Handle* handle, SPF_TruckData* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetTruckData();
    const auto& cpp_consts = pm.GetTelemetryService()->GetTruckConstants();

    // Placements and vectors
    out_data->world_placement = {{cpp_data.world_placement.position.x, cpp_data.world_placement.position.y, cpp_data.world_placement.position.z},
                               {cpp_data.world_placement.orientation.heading, cpp_data.world_placement.orientation.pitch, cpp_data.world_placement.orientation.roll}};
    out_data->local_linear_velocity = {cpp_data.local_linear_velocity.x, cpp_data.local_linear_velocity.y, cpp_data.local_linear_velocity.z};
    out_data->local_angular_velocity = {cpp_data.local_angular_velocity.x, cpp_data.local_angular_velocity.y, cpp_data.local_angular_velocity.z};
    out_data->local_linear_acceleration = {cpp_data.local_linear_acceleration.x, cpp_data.local_linear_acceleration.y, cpp_data.local_linear_acceleration.z};
    out_data->local_angular_acceleration = {cpp_data.local_angular_acceleration.x, cpp_data.local_angular_acceleration.y, cpp_data.local_angular_acceleration.z};
    out_data->cabin_offset = {{cpp_data.cabin_offset.position.x, cpp_data.cabin_offset.position.y, cpp_data.cabin_offset.position.z},
                            {cpp_data.cabin_offset.orientation.heading, cpp_data.cabin_offset.orientation.pitch, cpp_data.cabin_offset.orientation.roll}};
    out_data->cabin_angular_velocity = {cpp_data.cabin_angular_velocity.x, cpp_data.cabin_angular_velocity.y, cpp_data.cabin_angular_velocity.z};
    out_data->cabin_angular_acceleration = {cpp_data.cabin_angular_acceleration.x, cpp_data.cabin_angular_acceleration.y, cpp_data.cabin_angular_acceleration.z};
    out_data->head_offset = {{cpp_data.head_offset.position.x, cpp_data.head_offset.position.y, cpp_data.head_offset.position.z},
                           {cpp_data.head_offset.orientation.heading, cpp_data.head_offset.orientation.pitch, cpp_data.head_offset.orientation.roll}};

    // Simple values
    out_data->speed = cpp_data.speed;
    out_data->engine_rpm = cpp_data.engine_rpm;
    out_data->gear = cpp_data.gear;
    out_data->displayed_gear = cpp_data.displayed_gear;
    out_data->input_steering = cpp_data.input_steering;
    out_data->input_throttle = cpp_data.input_throttle;
    out_data->input_brake = cpp_data.input_brake;
    out_data->input_clutch = cpp_data.input_clutch;
    out_data->effective_steering = cpp_data.effective_steering;
    out_data->effective_throttle = cpp_data.effective_throttle;
    out_data->effective_brake = cpp_data.effective_brake;
    out_data->effective_clutch = cpp_data.effective_clutch;
    out_data->cruise_control_speed = cpp_data.cruise_control_speed;
    out_data->hshifter_slot = cpp_data.hshifter_slot;
    out_data->parking_brake = cpp_data.parking_brake;
    out_data->motor_brake = cpp_data.motor_brake;
    out_data->retarder_level = cpp_data.retarder_level;
    out_data->air_pressure = cpp_data.air_pressure;
    out_data->air_pressure_warning = cpp_data.air_pressure_warning;
    out_data->air_pressure_emergency = cpp_data.air_pressure_emergency;
    out_data->brake_temperature = cpp_data.brake_temperature;
    out_data->fuel_amount = cpp_data.fuel_amount;
    out_data->fuel_warning = cpp_data.fuel_warning;
    out_data->fuel_average_consumption = cpp_data.fuel_average_consumption;
    out_data->fuel_range = cpp_data.fuel_range;
    out_data->adblue_amount = cpp_data.adblue_amount;
    out_data->adblue_warning = cpp_data.adblue_warning;
    out_data->adblue_average_consumption = cpp_data.adblue_average_consumption;
    out_data->oil_pressure = cpp_data.oil_pressure;
    out_data->oil_pressure_warning = cpp_data.oil_pressure_warning;
    out_data->oil_temperature = cpp_data.oil_temperature;
    out_data->water_temperature = cpp_data.water_temperature;
    out_data->water_temperature_warning = cpp_data.water_temperature_warning;
    out_data->battery_voltage = cpp_data.battery_voltage;
    out_data->battery_voltage_warning = cpp_data.battery_voltage_warning;
    out_data->electric_enabled = cpp_data.electric_enabled;
    out_data->engine_enabled = cpp_data.engine_enabled;
    out_data->wipers = cpp_data.wipers;
    out_data->differential_lock = cpp_data.differential_lock;
    out_data->lift_axle = cpp_data.lift_axle;
    out_data->lift_axle_indicator = cpp_data.lift_axle_indicator;
    out_data->trailer_lift_axle = cpp_data.trailer_lift_axle;
    out_data->trailer_lift_axle_indicator = cpp_data.trailer_lift_axle_indicator;
    out_data->lblinker = cpp_data.lblinker;
    out_data->rblinker = cpp_data.rblinker;
    out_data->hazard_warning = cpp_data.hazard_warning;
    out_data->light_lblinker = cpp_data.light_lblinker;
    out_data->light_rblinker = cpp_data.light_rblinker;
    out_data->light_parking = cpp_data.light_parking;
    out_data->light_low_beam = cpp_data.light_low_beam;
    out_data->light_high_beam = cpp_data.light_high_beam;
    out_data->light_aux_front = cpp_data.light_aux_front;
    out_data->light_aux_roof = cpp_data.light_aux_roof;
    out_data->light_beacon = cpp_data.light_beacon;
    out_data->light_brake = cpp_data.light_brake;
    out_data->light_reverse = cpp_data.light_reverse;
    out_data->dashboard_backlight = cpp_data.dashboard_backlight;
    out_data->wear_engine = cpp_data.wear_engine;
    out_data->wear_transmission = cpp_data.wear_transmission;
    out_data->wear_cabin = cpp_data.wear_cabin;
    out_data->wear_chassis = cpp_data.wear_chassis;
    out_data->wear_wheels = cpp_data.wear_wheels;
    out_data->odometer = cpp_data.odometer;

    // Arrays
    uint32_t selectors_to_copy = std::min<uint32_t>(cpp_consts.selector_count, SPF_TELEMETRY_SELECTOR_MAX_COUNT);
    for (uint32_t i = 0; i < selectors_to_copy; ++i) {
        out_data->hshifter_selector[i] = cpp_data.hshifter_selector[i];
    }

    uint32_t wheels_to_copy = std::min<uint32_t>(cpp_consts.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
    for (uint32_t i = 0; i < wheels_to_copy; ++i) {
        out_data->wheels[i].suspension_deflection = cpp_data.wheels[i].suspension_deflection;
        out_data->wheels[i].on_ground = cpp_data.wheels[i].on_ground;
        out_data->wheels[i].substance = cpp_data.wheels[i].substance;
        out_data->wheels[i].angular_velocity = cpp_data.wheels[i].angular_velocity;
        out_data->wheels[i].steering = cpp_data.wheels[i].steering;
        out_data->wheels[i].rotation = cpp_data.wheels[i].rotation;
        out_data->wheels[i].lift = cpp_data.wheels[i].lift;
        out_data->wheels[i].lift_offset = cpp_data.wheels[i].lift_offset;
    }
}

void TelemetryApi::T_GetTrailers(SPF_Telemetry_Handle* handle, SPF_Trailer* out_trailers, uint32_t* in_out_count) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_trailers || !in_out_count || !pm.GetTelemetryService()) return;

    const auto& cpp_trailers = pm.GetTelemetryService()->GetTrailers();
    uint32_t max_count = *in_out_count;
    uint32_t trailers_to_copy = std::min<uint32_t>(static_cast<uint32_t>(cpp_trailers.size()), max_count);

    for (uint32_t i = 0; i < trailers_to_copy; ++i) {
        const auto& cpp_trailer = cpp_trailers[i];
        SPF_Trailer& c_trailer = out_trailers[i];

        // Copy constants
        auto& cpp_consts = cpp_trailer.constants;
        auto& c_consts = c_trailer.constants;
        strcpy_s(c_consts.id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.id.c_str());
        strcpy_s(c_consts.cargo_accessory_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.cargo_accessory_id.c_str());
        strcpy_s(c_consts.brand_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.brand_id.c_str());
        strcpy_s(c_consts.brand, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.brand.c_str());
        strcpy_s(c_consts.name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.name.c_str());
        strcpy_s(c_consts.chain_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.chain_type.c_str());
        strcpy_s(c_consts.body_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.body_type.c_str());
        strcpy_s(c_consts.license_plate, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.license_plate.c_str());
        strcpy_s(c_consts.license_plate_country_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.license_plate_country_id.c_str());
        strcpy_s(c_consts.license_plate_country, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.license_plate_country.c_str());
        c_consts.hook_position = {cpp_consts.hook_position.x, cpp_consts.hook_position.y, cpp_consts.hook_position.z};
        c_consts.wheel_count = std::min<uint32_t>(cpp_consts.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
        for (uint32_t j = 0; j < c_consts.wheel_count; ++j) {
            c_consts.wheels[j].simulated = cpp_consts.wheels[j].simulated;
            c_consts.wheels[j].powered = cpp_consts.wheels[j].powered;
            c_consts.wheels[j].steerable = cpp_consts.wheels[j].steerable;
            c_consts.wheels[j].liftable = cpp_consts.wheels[j].liftable;
            c_consts.wheels[j].radius = cpp_consts.wheels[j].radius;
            c_consts.wheels[j].position = {cpp_consts.wheels[j].position.x, cpp_consts.wheels[j].position.y, cpp_consts.wheels[j].position.z};
        }

        // Copy data
        auto& cpp_data = cpp_trailer.data;
        auto& c_data = c_trailer.data;
        c_data.connected = cpp_data.connected;
        c_data.cargo_damage = cpp_data.cargo_damage;
        c_data.world_placement = {{cpp_data.world_placement.position.x, cpp_data.world_placement.position.y, cpp_data.world_placement.position.z},
                                  {cpp_data.world_placement.orientation.heading, cpp_data.world_placement.orientation.pitch, cpp_data.world_placement.orientation.roll}};
        c_data.local_linear_velocity = {cpp_data.local_linear_velocity.x, cpp_data.local_linear_velocity.y, cpp_data.local_linear_velocity.z};
        c_data.local_angular_velocity = {cpp_data.local_angular_velocity.x, cpp_data.local_angular_velocity.y, cpp_data.local_angular_velocity.z};
        c_data.local_linear_acceleration = {cpp_data.local_linear_acceleration.x, cpp_data.local_linear_acceleration.y, cpp_data.local_linear_acceleration.z};
        c_data.local_angular_acceleration = {cpp_data.local_angular_acceleration.x, cpp_data.local_angular_acceleration.y, cpp_data.local_angular_acceleration.z};
        c_data.wear_body = cpp_data.wear_body;
        c_data.wear_chassis = cpp_data.wear_chassis;
        c_data.wear_wheels = cpp_data.wear_wheels;
        for (uint32_t j = 0; j < c_consts.wheel_count; ++j) {
            c_data.wheels[j].suspension_deflection = cpp_data.wheels[j].suspension_deflection;
            c_data.wheels[j].on_ground = cpp_data.wheels[j].on_ground;
            c_data.wheels[j].substance = cpp_data.wheels[j].substance;
            c_data.wheels[j].angular_velocity = cpp_data.wheels[j].angular_velocity;
            c_data.wheels[j].steering = cpp_data.wheels[j].steering;
            c_data.wheels[j].rotation = cpp_data.wheels[j].rotation;
            c_data.wheels[j].lift = cpp_data.wheels[j].lift;
            c_data.wheels[j].lift_offset = cpp_data.wheels[j].lift_offset;
        }
    }

    *in_out_count = trailers_to_copy;
}

void TelemetryApi::T_GetJobConstants(SPF_Telemetry_Handle* handle, SPF_JobConstants* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetJobConstants();

    out_data->income = cpp_data.income;
    out_data->delivery_time = cpp_data.delivery_time;
    out_data->planned_distance_km = cpp_data.planned_distance_km;
    out_data->is_cargo_loaded = cpp_data.is_cargo_loaded;
    out_data->is_special_job = cpp_data.is_special_job;
    strcpy_s(out_data->job_market, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.job_market.c_str());
    strcpy_s(out_data->cargo_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.cargo_id.c_str());
    strcpy_s(out_data->cargo_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.cargo_name.c_str());
    out_data->cargo_mass = cpp_data.cargo_mass;
    out_data->cargo_unit_count = cpp_data.cargo_unit_count;
    out_data->cargo_unit_mass = cpp_data.cargo_unit_mass;
    strcpy_s(out_data->destination_city_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.destination_city_id.c_str());
    strcpy_s(out_data->destination_city, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.destination_city.c_str());
    strcpy_s(out_data->destination_company_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.destination_company_id.c_str());
    strcpy_s(out_data->destination_company, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.destination_company.c_str());
    strcpy_s(out_data->source_city_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.source_city_id.c_str());
    strcpy_s(out_data->source_city, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.source_city.c_str());
    strcpy_s(out_data->source_company_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.source_company_id.c_str());
    strcpy_s(out_data->source_company, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.source_company.c_str());
}

void TelemetryApi::T_GetJobData(SPF_Telemetry_Handle* handle, SPF_JobData* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetJobData();
    out_data->on_job = cpp_data.on_job;
    out_data->cargo_damage = cpp_data.cargo_damage;
    out_data->remaining_delivery_minutes = cpp_data.remaining_delivery_minutes;
}

void TelemetryApi::T_GetNavigationData(SPF_Telemetry_Handle* handle, SPF_NavigationData* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetNavigationData();
    out_data->navigation_distance = cpp_data.navigation_distance;
    out_data->navigation_time = cpp_data.navigation_time;
    out_data->navigation_speed_limit = cpp_data.navigation_speed_limit;
    out_data->navigation_time_real_seconds = cpp_data.navigation_time_real_seconds;
}

void TelemetryApi::T_GetControls(SPF_Telemetry_Handle* handle, SPF_Controls* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetControls();
    out_data->userInput.steering = cpp_data.userInput.steering;
    out_data->userInput.throttle = cpp_data.userInput.throttle;
    out_data->userInput.brake = cpp_data.userInput.brake;
    out_data->userInput.clutch = cpp_data.userInput.clutch;
    out_data->effectiveInput.steering = cpp_data.effectiveInput.steering;
    out_data->effectiveInput.throttle = cpp_data.effectiveInput.throttle;
    out_data->effectiveInput.brake = cpp_data.effectiveInput.brake;
    out_data->effectiveInput.clutch = cpp_data.effectiveInput.clutch;
}

void TelemetryApi::T_GetSpecialEvents(SPF_Telemetry_Handle* handle, SPF_SpecialEvents* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetSpecialEvents();
    out_data->job_delivered = cpp_data.job_delivered;
    out_data->job_cancelled = cpp_data.job_cancelled;
    out_data->fined = cpp_data.fined;
    out_data->tollgate = cpp_data.tollgate;
    out_data->ferry = cpp_data.ferry;
    out_data->train = cpp_data.train;
}

void TelemetryApi::T_GetGameplayEvents(SPF_Telemetry_Handle* handle, SPF_GameplayEvents* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetGameplayEvents();

    // Job Delivered
    out_data->job_delivered.revenue = cpp_data.job_delivered.revenue;
    out_data->job_delivered.earned_xp = cpp_data.job_delivered.earned_xp;
    out_data->job_delivered.cargo_damage = cpp_data.job_delivered.cargo_damage;
    out_data->job_delivered.distance_km = cpp_data.job_delivered.distance_km;
    out_data->job_delivered.delivery_time = cpp_data.job_delivered.delivery_time;
    out_data->job_delivered.auto_park_used = cpp_data.job_delivered.auto_park_used;
    out_data->job_delivered.auto_load_used = cpp_data.job_delivered.auto_load_used;

    // Job Cancelled
    out_data->job_cancelled.penalty = cpp_data.job_cancelled.penalty;

    // Player Fined
    out_data->player_fined.fine_amount = cpp_data.player_fined.fine_amount;
    strcpy_s(out_data->player_fined.fine_offence, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.player_fined.fine_offence.c_str());

    // Tollgate Paid
    out_data->tollgate_paid.pay_amount = cpp_data.tollgate_paid.pay_amount;

    // Ferry Used
    out_data->ferry_used.pay_amount = cpp_data.ferry_used.pay_amount;
    strcpy_s(out_data->ferry_used.source_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.ferry_used.source_name.c_str());
    strcpy_s(out_data->ferry_used.target_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.ferry_used.target_name.c_str());
    strcpy_s(out_data->ferry_used.source_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.ferry_used.source_id.c_str());
    strcpy_s(out_data->ferry_used.target_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.ferry_used.target_id.c_str());

    // Train Used
    out_data->train_used.pay_amount = cpp_data.train_used.pay_amount;
    strcpy_s(out_data->train_used.source_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.train_used.source_name.c_str());
    strcpy_s(out_data->train_used.target_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.train_used.target_name.c_str());
    strcpy_s(out_data->train_used.source_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.train_used.source_id.c_str());
    strcpy_s(out_data->train_used.target_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.train_used.target_id.c_str());
}

void TelemetryApi::T_GetGearboxConstants(SPF_Telemetry_Handle* handle, SPF_GearboxConstants* out_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_data || !pm.GetTelemetryService()) return;

    const auto& cpp_data = pm.GetTelemetryService()->GetGearboxConstants();

    strcpy_s(out_data->shifter_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.shifter_type.c_str());

    out_data->slot_count = static_cast<uint32_t>(std::min<size_t>(cpp_data.slot_gear.size(), SPF_TELEMETRY_HSHIFTER_MAX_SLOTS));
    for (uint32_t i = 0; i < out_data->slot_count; ++i) {
        out_data->slot_gear[i] = cpp_data.slot_gear[i];
        out_data->slot_handle_position[i] = cpp_data.slot_handle_position[i];
        out_data->slot_selectors[i] = cpp_data.slot_selectors[i];
    }
}

int TelemetryApi::T_GetLastGameplayEventId(SPF_Telemetry_Handle* handle, char* out_buffer, int buffer_size) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !out_buffer || buffer_size <= 0 || !pm.GetTelemetryService()) return 0;

    const auto& event_id = pm.GetTelemetryService()->GetLastGameplayEventId();
    if (event_id.length() < buffer_size) {
        strcpy_s(out_buffer, buffer_size, event_id.c_str());
        return event_id.length();
    } else {
        *out_buffer = '\0';
        return event_id.length() + 1;  // Return required size
    }
}

// --- Event-Driven Callback Invocation & Conversion ---
void TelemetryApi::InvokeGameStateCallback(const GameState& cpp_data, SPF_Telemetry_GameState_Callback callback, void* user_data) {
    // Create a C-style struct for output
    SPF_GameState c_data;
    // Perform conversion from cpp_data to c_data
    strcpy_s(c_data.game_id, SPF_TELEMETRY_ID_MAX_SIZE, GameToString(cpp_data.game_id));
    strcpy_s(c_data.game_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.game_name.c_str());
    
    c_data.scs_game_version_major = cpp_data.scs_game_version_major;
    c_data.scs_game_version_minor = cpp_data.scs_game_version_minor;
    
    c_data.telemetry_plugin_version_major = cpp_data.telemetry_plugin_version_major;
    c_data.telemetry_plugin_version_minor = cpp_data.telemetry_plugin_version_minor;

    c_data.telemetry_game_version_major = cpp_data.telemetry_game_version_major;
    c_data.telemetry_game_version_minor = cpp_data.telemetry_game_version_minor;

    c_data.paused = cpp_data.paused;
    c_data.scale = cpp_data.scale;
    c_data.multiplayer_time_offset = cpp_data.multiplayer_time_offset;

    // Invoke the plugin's callback
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeTimestampsCallback(const Timestamps& cpp_data, SPF_Telemetry_Timestamps_Callback callback, void* user_data) {
    SPF_Timestamps c_data;
    c_data.simulation = cpp_data.simulation;
    c_data.render = cpp_data.render;
    c_data.paused_simulation = cpp_data.paused_simulation; // Added missing line
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeCommonDataCallback(const CommonData& cpp_data, SPF_Telemetry_CommonData_Callback callback, void* user_data) {
    SPF_CommonData c_data;
    c_data.game_time = cpp_data.game_time;
    c_data.next_rest_stop = cpp_data.next_rest_stop;
    c_data.next_rest_stop_time.DayOfWeek = cpp_data.next_rest_stop_time.DayOfWeek;
    c_data.next_rest_stop_time.Hour = cpp_data.next_rest_stop_time.Hour;
    c_data.next_rest_stop_time.Minute = cpp_data.next_rest_stop_time.Minute;
    c_data.next_rest_stop_real_minutes = cpp_data.next_rest_stop_real_minutes;

    c_data.substance_count = static_cast<uint32_t>(std::min<size_t>(cpp_data.substances.size(), SPF_TELEMETRY_SUBSTANCE_MAX_COUNT));
    for (uint32_t i = 0; i < c_data.substance_count; ++i) {
        strcpy_s(c_data.substances[i], SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.substances[i].c_str());
    }
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeTruckConstantsCallback(const TruckConstants& cpp_data, SPF_Telemetry_TruckConstants_Callback callback, void* user_data) {
    SPF_TruckConstants c_data;

    strcpy_s(c_data.brand_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.brand_id.c_str());
    strcpy_s(c_data.brand, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.brand.c_str());
    strcpy_s(c_data.id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.id.c_str());
    strcpy_s(c_data.name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.name.c_str());
    strcpy_s(c_data.license_plate, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate.c_str());
    strcpy_s(c_data.license_plate_country_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate_country_id.c_str());
    strcpy_s(c_data.license_plate_country, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.license_plate_country.c_str());

    c_data.fuel_capacity = cpp_data.fuel_capacity;
    c_data.fuel_warning_factor = cpp_data.fuel_warning_factor;
    c_data.adblue_capacity = cpp_data.adblue_capacity;
    c_data.adblue_warning_factor = cpp_data.adblue_warning_factor;
    c_data.air_pressure_warning = cpp_data.air_pressure_warning;
    c_data.air_pressure_emergency = cpp_data.air_pressure_emergency;
    c_data.oil_pressure_warning = cpp_data.oil_pressure_warning;
    c_data.water_temperature_warning = cpp_data.water_temperature_warning;
    c_data.battery_voltage_warning = cpp_data.battery_voltage_warning;
    c_data.rpm_limit = cpp_data.rpm_limit;
    c_data.forward_gear_count = cpp_data.forward_gear_count;
    c_data.reverse_gear_count = cpp_data.reverse_gear_count;
    c_data.retarder_step_count = cpp_data.retarder_step_count;
    c_data.selector_count = cpp_data.selector_count;
    c_data.differential_ratio = cpp_data.differential_ratio;

    c_data.cabin_position = {cpp_data.cabin_position.x, cpp_data.cabin_position.y, cpp_data.cabin_position.z};
    c_data.head_position = {cpp_data.head_position.x, cpp_data.head_position.y, cpp_data.head_position.z};
    c_data.hook_position = {cpp_data.hook_position.x, cpp_data.hook_position.y, cpp_data.hook_position.z};

    c_data.wheel_count = std::min<uint32_t>(cpp_data.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
    for (uint32_t i = 0; i < c_data.wheel_count; ++i) {
        c_data.wheels[i].simulated = cpp_data.wheels[i].simulated;
        c_data.wheels[i].powered = cpp_data.wheels[i].powered;
        c_data.wheels[i].steerable = cpp_data.wheels[i].steerable;
        c_data.wheels[i].liftable = cpp_data.wheels[i].liftable;
        c_data.wheels[i].radius = cpp_data.wheels[i].radius;
        c_data.wheels[i].position = {cpp_data.wheels[i].position.x, cpp_data.wheels[i].position.y, cpp_data.wheels[i].position.z};
    }

    uint32_t forward_gears_to_copy = std::min<uint32_t>(cpp_data.forward_gear_count, SPF_TELEMETRY_GEAR_MAX_COUNT);
    memcpy(c_data.gear_ratios_forward, cpp_data.gear_ratios_forward.data(), forward_gears_to_copy * sizeof(float));

    uint32_t reverse_gears_to_copy = std::min<uint32_t>(cpp_data.reverse_gear_count, SPF_TELEMETRY_GEAR_MAX_COUNT);
    memcpy(c_data.gear_ratios_reverse, cpp_data.gear_ratios_reverse.data(), reverse_gears_to_copy * sizeof(float));
    
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeTrailerConstantsCallback(const TrailerConstants& cpp_data, SPF_Telemetry_TrailerConstants_Callback callback, void* user_data) {
    SPF_TrailerConstants c_data;

    strcpy_s(c_data.id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.id.c_str());
    strcpy_s(c_data.cargo_accessory_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.cargo_accessory_id.c_str());
    strcpy_s(c_data.brand_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.brand_id.c_str());
    strcpy_s(c_data.brand, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.brand.c_str());
    strcpy_s(c_data.name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.name.c_str());
    strcpy_s(c_data.chain_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.chain_type.c_str());
    strcpy_s(c_data.body_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.body_type.c_str());
    strcpy_s(c_data.license_plate, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate.c_str());
    strcpy_s(c_data.license_plate_country_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.license_plate_country_id.c_str());
    strcpy_s(c_data.license_plate_country, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.license_plate_country.c_str());
    c_data.hook_position = {cpp_data.hook_position.x, cpp_data.hook_position.y, cpp_data.hook_position.z};
    c_data.wheel_count = std::min<uint32_t>(cpp_data.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
    for (uint32_t i = 0; i < c_data.wheel_count; ++i) {
        c_data.wheels[i].simulated = cpp_data.wheels[i].simulated;
        c_data.wheels[i].powered = cpp_data.wheels[i].powered;
        c_data.wheels[i].steerable = cpp_data.wheels[i].steerable;
        c_data.wheels[i].liftable = cpp_data.wheels[i].liftable;
        c_data.wheels[i].radius = cpp_data.wheels[i].radius;
        c_data.wheels[i].position = {cpp_data.wheels[i].position.x, cpp_data.wheels[i].position.y, cpp_data.wheels[i].position.z};
    }

    callback(&c_data, user_data);
}

void TelemetryApi::InvokeTruckDataCallback(const TruckData& cpp_data, SPF_Telemetry_TruckData_Callback callback, void* user_data) {
    SPF_TruckData c_data;
    auto& pm = PluginManager::GetInstance();
    const auto& cpp_consts = pm.GetTelemetryService()->GetTruckConstants(); // Need constants for array sizes

    // Placements and vectors
    c_data.world_placement = {{cpp_data.world_placement.position.x, cpp_data.world_placement.position.y, cpp_data.world_placement.position.z},
                              {cpp_data.world_placement.orientation.heading, cpp_data.world_placement.orientation.pitch, cpp_data.world_placement.orientation.roll}};
    c_data.local_linear_velocity = {cpp_data.local_linear_velocity.x, cpp_data.local_linear_velocity.y, cpp_data.local_linear_velocity.z};
    c_data.local_angular_velocity = {cpp_data.local_angular_velocity.x, cpp_data.local_angular_velocity.y, cpp_data.local_angular_velocity.z};
    c_data.local_linear_acceleration = {cpp_data.local_linear_acceleration.x, cpp_data.local_linear_acceleration.y, cpp_data.local_linear_acceleration.z};
    c_data.local_angular_acceleration = {cpp_data.local_angular_acceleration.x, cpp_data.local_angular_acceleration.y, cpp_data.local_angular_acceleration.z};
    c_data.cabin_offset = {{cpp_data.cabin_offset.position.x, cpp_data.cabin_offset.position.y, cpp_data.cabin_offset.position.z},
                           {cpp_data.cabin_offset.orientation.heading, cpp_data.cabin_offset.orientation.pitch, cpp_data.cabin_offset.orientation.roll}};
    c_data.cabin_angular_velocity = {cpp_data.cabin_angular_velocity.x, cpp_data.cabin_angular_velocity.y, cpp_data.cabin_angular_velocity.z};
    c_data.cabin_angular_acceleration = {cpp_data.cabin_angular_acceleration.x, cpp_data.cabin_angular_acceleration.y, cpp_data.cabin_angular_acceleration.z};
    c_data.head_offset = {{cpp_data.head_offset.position.x, cpp_data.head_offset.position.y, cpp_data.head_offset.position.z},
                          {cpp_data.head_offset.orientation.heading, cpp_data.head_offset.orientation.pitch, cpp_data.head_offset.orientation.roll}};

    // Simple values
    c_data.speed = cpp_data.speed;
    c_data.engine_rpm = cpp_data.engine_rpm;
    c_data.gear = cpp_data.gear;
    c_data.displayed_gear = cpp_data.displayed_gear;
    c_data.input_steering = cpp_data.input_steering;
    c_data.input_throttle = cpp_data.input_throttle;
    c_data.input_brake = cpp_data.input_brake;
    c_data.input_clutch = cpp_data.input_clutch;
    c_data.effective_steering = cpp_data.effective_steering;
    c_data.effective_throttle = cpp_data.effective_throttle;
    c_data.effective_brake = cpp_data.effective_brake;
    c_data.effective_clutch = cpp_data.effective_clutch;
    c_data.cruise_control_speed = cpp_data.cruise_control_speed;
    c_data.hshifter_slot = cpp_data.hshifter_slot;
    c_data.parking_brake = cpp_data.parking_brake;
    c_data.motor_brake = cpp_data.motor_brake;
    c_data.retarder_level = cpp_data.retarder_level;
    c_data.air_pressure = cpp_data.air_pressure;
    c_data.air_pressure_warning = cpp_data.air_pressure_warning;
    c_data.air_pressure_emergency = cpp_data.air_pressure_emergency;
    c_data.brake_temperature = cpp_data.brake_temperature;
    c_data.fuel_amount = cpp_data.fuel_amount;
    c_data.fuel_warning = cpp_data.fuel_warning;
    c_data.fuel_average_consumption = cpp_data.fuel_average_consumption;
    c_data.fuel_range = cpp_data.fuel_range;
    c_data.adblue_amount = cpp_data.adblue_amount;
    c_data.adblue_warning = cpp_data.adblue_warning;
    c_data.adblue_average_consumption = cpp_data.adblue_average_consumption;
    c_data.oil_pressure = cpp_data.oil_pressure;
    c_data.oil_pressure_warning = cpp_data.oil_pressure_warning;
    c_data.oil_temperature = cpp_data.oil_temperature;
    c_data.water_temperature = cpp_data.water_temperature;
    c_data.water_temperature_warning = cpp_data.water_temperature_warning;
    c_data.battery_voltage = cpp_data.battery_voltage;
    c_data.battery_voltage_warning = cpp_data.battery_voltage_warning;
    c_data.electric_enabled = cpp_data.electric_enabled;
    c_data.engine_enabled = cpp_data.engine_enabled;
    c_data.wipers = cpp_data.wipers;
    c_data.differential_lock = cpp_data.differential_lock;
    c_data.lift_axle = cpp_data.lift_axle;
    c_data.lift_axle_indicator = cpp_data.lift_axle_indicator;
    c_data.trailer_lift_axle = cpp_data.trailer_lift_axle;
    c_data.trailer_lift_axle_indicator = cpp_data.trailer_lift_axle_indicator;
    c_data.lblinker = cpp_data.lblinker;
    c_data.rblinker = cpp_data.rblinker;
    c_data.hazard_warning = cpp_data.hazard_warning;
    c_data.light_lblinker = cpp_data.light_lblinker;
    c_data.light_rblinker = cpp_data.light_rblinker;
    c_data.light_parking = cpp_data.light_parking;
    c_data.light_low_beam = cpp_data.light_low_beam;
    c_data.light_high_beam = cpp_data.light_high_beam;
    c_data.light_aux_front = cpp_data.light_aux_front;
    c_data.light_aux_roof = cpp_data.light_aux_roof;
    c_data.light_beacon = cpp_data.light_beacon;
    c_data.light_brake = cpp_data.light_brake;
    c_data.light_reverse = cpp_data.light_reverse;
    c_data.dashboard_backlight = cpp_data.dashboard_backlight;
    c_data.wear_engine = cpp_data.wear_engine;
    c_data.wear_transmission = cpp_data.wear_transmission;
    c_data.wear_cabin = cpp_data.wear_cabin;
    c_data.wear_chassis = cpp_data.wear_chassis;
    c_data.wear_wheels = cpp_data.wear_wheels;
    c_data.odometer = cpp_data.odometer;

    // Arrays
    uint32_t selectors_to_copy = std::min<uint32_t>(cpp_consts.selector_count, SPF_TELEMETRY_SELECTOR_MAX_COUNT);
    for (uint32_t i = 0; i < selectors_to_copy; ++i) {
        c_data.hshifter_selector[i] = cpp_data.hshifter_selector[i];
    }

    uint32_t wheels_to_copy = std::min<uint32_t>(cpp_consts.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
    for (uint32_t i = 0; i < wheels_to_copy; ++i) {
        c_data.wheels[i].suspension_deflection = cpp_data.wheels[i].suspension_deflection;
        c_data.wheels[i].on_ground = cpp_data.wheels[i].on_ground;
        c_data.wheels[i].substance = cpp_data.wheels[i].substance;
        c_data.wheels[i].angular_velocity = cpp_data.wheels[i].angular_velocity;
        c_data.wheels[i].steering = cpp_data.wheels[i].steering;
        c_data.wheels[i].rotation = cpp_data.wheels[i].rotation;
        c_data.wheels[i].lift = cpp_data.wheels[i].lift;
        c_data.wheels[i].lift_offset = cpp_data.wheels[i].lift_offset;
    }

    callback(&c_data, user_data);
}

void TelemetryApi::InvokeTrailersCallback(const std::vector<Trailer>& cpp_trailers, SPF_Telemetry_Trailers_Callback callback, void* user_data) {
    // Filter the list of trailers to include only 'active' ones.
    // An active trailer is one that is either connected or has a valid ID.
    std::vector<Trailer> active_trailers;
    for (const auto& trailer : cpp_trailers) {
        if (trailer.data.connected || !trailer.constants.id.empty()) {
            active_trailers.push_back(trailer);
        }
    }

    if (active_trailers.empty()) {
        callback(nullptr, 0, user_data);
        return;
    }

    // Create a C-style array for output from the filtered list.
    auto c_trailers = std::make_unique<SPF_Trailer[]>(active_trailers.size());
    uint32_t trailer_count = static_cast<uint32_t>(active_trailers.size());

    for (uint32_t i = 0; i < trailer_count; ++i) {
        const auto& cpp_trailer = active_trailers[i];
        SPF_Trailer& c_trailer = c_trailers[i];

        // Copy constants
        const auto& cpp_consts = cpp_trailer.constants;
        auto& c_consts = c_trailer.constants;
        strcpy_s(c_consts.id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.id.c_str());
        strcpy_s(c_consts.cargo_accessory_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.cargo_accessory_id.c_str());
        strcpy_s(c_consts.brand_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.brand_id.c_str());
        strcpy_s(c_consts.brand, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.brand.c_str());
        strcpy_s(c_consts.name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.name.c_str());
        strcpy_s(c_consts.chain_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.chain_type.c_str());
        strcpy_s(c_consts.body_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.body_type.c_str());
        strcpy_s(c_consts.license_plate, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.license_plate.c_str());
        strcpy_s(c_consts.license_plate_country_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_consts.license_plate_country_id.c_str());
        strcpy_s(c_consts.license_plate_country, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_consts.license_plate_country.c_str());
        c_consts.hook_position = {cpp_consts.hook_position.x, cpp_consts.hook_position.y, cpp_consts.hook_position.z};
        c_consts.wheel_count = std::min<uint32_t>(cpp_consts.wheel_count, SPF_TELEMETRY_WHEEL_MAX_COUNT);
        for (uint32_t j = 0; j < c_consts.wheel_count; ++j) {
            c_consts.wheels[j].simulated = cpp_consts.wheels[j].simulated;
            c_consts.wheels[j].powered = cpp_consts.wheels[j].powered;
            c_consts.wheels[j].steerable = cpp_consts.wheels[j].steerable;
            c_consts.wheels[j].liftable = cpp_consts.wheels[j].liftable;
            c_consts.wheels[j].radius = cpp_consts.wheels[j].radius;
            c_consts.wheels[j].position = {cpp_consts.wheels[j].position.x, cpp_consts.wheels[j].position.y, cpp_consts.wheels[j].position.z};
        }

        // Copy data
        const auto& cpp_data = cpp_trailer.data;
        auto& c_data = c_trailer.data;
        c_data.connected = cpp_data.connected;
        c_data.cargo_damage = cpp_data.cargo_damage;
        c_data.world_placement = {{cpp_data.world_placement.position.x, cpp_data.world_placement.position.y, cpp_data.world_placement.position.z},
                                  {cpp_data.world_placement.orientation.heading, cpp_data.world_placement.orientation.pitch, cpp_data.world_placement.orientation.roll}};
        c_data.local_linear_velocity = {cpp_data.local_linear_velocity.x, cpp_data.local_linear_velocity.y, cpp_data.local_linear_velocity.z};
        c_data.local_angular_velocity = {cpp_data.local_angular_velocity.x, cpp_data.local_angular_velocity.y, cpp_data.local_angular_velocity.z};
        c_data.local_linear_acceleration = {cpp_data.local_linear_acceleration.x, cpp_data.local_linear_acceleration.y, cpp_data.local_linear_acceleration.z};
        c_data.local_angular_acceleration = {cpp_data.local_angular_acceleration.x, cpp_data.local_angular_acceleration.y, cpp_data.local_angular_acceleration.z};
        c_data.wear_body = cpp_data.wear_body;
        c_data.wear_chassis = cpp_data.wear_chassis;
        c_data.wear_wheels = cpp_data.wear_wheels;
        for (uint32_t j = 0; j < c_consts.wheel_count; ++j) {
            c_data.wheels[j].suspension_deflection = cpp_data.wheels[j].suspension_deflection;
            c_data.wheels[j].on_ground = cpp_data.wheels[j].on_ground;
            c_data.wheels[j].substance = cpp_data.wheels[j].substance;
            c_data.wheels[j].angular_velocity = cpp_data.wheels[j].angular_velocity;
            c_data.wheels[j].steering = cpp_data.wheels[j].steering;
            c_data.wheels[j].rotation = cpp_data.wheels[j].rotation;
            c_data.wheels[j].lift = cpp_data.wheels[j].lift;
            c_data.wheels[j].lift_offset = cpp_data.wheels[j].lift_offset;
        }
    }
    
    callback(c_trailers.get(), trailer_count, user_data);
}

void TelemetryApi::InvokeJobConstantsCallback(const JobConstants& cpp_data, SPF_Telemetry_JobConstants_Callback callback, void* user_data) {
    SPF_JobConstants c_data;
    
    c_data.income = cpp_data.income;
    c_data.delivery_time = cpp_data.delivery_time;
    c_data.planned_distance_km = cpp_data.planned_distance_km;
    c_data.is_cargo_loaded = cpp_data.is_cargo_loaded;
    c_data.is_special_job = cpp_data.is_special_job;
    strcpy_s(c_data.job_market, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.job_market.c_str());
    strcpy_s(c_data.cargo_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.cargo_id.c_str());
    strcpy_s(c_data.cargo_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.cargo_name.c_str());
    c_data.cargo_mass = cpp_data.cargo_mass;
    c_data.cargo_unit_count = cpp_data.cargo_unit_count;
    c_data.cargo_unit_mass = cpp_data.cargo_unit_mass;
    strcpy_s(c_data.destination_city_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.destination_city_id.c_str());
    strcpy_s(c_data.destination_city, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.destination_city.c_str());
    strcpy_s(c_data.destination_company_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.destination_company_id.c_str());
    strcpy_s(c_data.destination_company, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.destination_company.c_str());
    strcpy_s(c_data.source_city_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.source_city_id.c_str());
    strcpy_s(c_data.source_city, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.source_city.c_str());
    strcpy_s(c_data.source_company_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.source_company_id.c_str());
    strcpy_s(c_data.source_company, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.source_company.c_str());

    callback(&c_data, user_data);
}

void TelemetryApi::InvokeJobDataCallback(const JobData& cpp_data, SPF_Telemetry_JobData_Callback callback, void* user_data) {
    SPF_JobData c_data;
    c_data.on_job = cpp_data.on_job;
    c_data.cargo_damage = cpp_data.cargo_damage;
    c_data.remaining_delivery_minutes = cpp_data.remaining_delivery_minutes;
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeNavigationDataCallback(const NavigationData& cpp_data, SPF_Telemetry_NavigationData_Callback callback, void* user_data) {
    SPF_NavigationData c_data;
    c_data.navigation_distance = cpp_data.navigation_distance;
    c_data.navigation_time = cpp_data.navigation_time;
    c_data.navigation_speed_limit = cpp_data.navigation_speed_limit;
    c_data.navigation_time_real_seconds = cpp_data.navigation_time_real_seconds;
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeControlsCallback(const Controls& cpp_data, SPF_Telemetry_Controls_Callback callback, void* user_data) {
    SPF_Controls c_data;
    c_data.userInput.steering = cpp_data.userInput.steering;
    c_data.userInput.throttle = cpp_data.userInput.throttle;
    c_data.userInput.brake = cpp_data.userInput.brake;
    c_data.userInput.clutch = cpp_data.userInput.clutch;
    c_data.effectiveInput.steering = cpp_data.effectiveInput.steering;
    c_data.effectiveInput.throttle = cpp_data.effectiveInput.throttle;
    c_data.effectiveInput.brake = cpp_data.effectiveInput.brake;
    c_data.effectiveInput.clutch = cpp_data.effectiveInput.clutch;
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeSpecialEventsCallback(const SpecialEvents& cpp_data, SPF_Telemetry_SpecialEvents_Callback callback, void* user_data) {
    SPF_SpecialEvents c_data;
    c_data.job_delivered = cpp_data.job_delivered;
    c_data.job_cancelled = cpp_data.job_cancelled;
    c_data.fined = cpp_data.fined;
    c_data.tollgate = cpp_data.tollgate;
    c_data.ferry = cpp_data.ferry;
    c_data.train = cpp_data.train;
    callback(&c_data, user_data);
}

void TelemetryApi::InvokeGameplayEventsCallback(const char* event_id, const GameplayEvents& cpp_data, SPF_Telemetry_GameplayEvents_Callback callback, void* user_data) {
    SPF_GameplayEvents c_data;

    // Job Delivered
    c_data.job_delivered.revenue = cpp_data.job_delivered.revenue;
    c_data.job_delivered.earned_xp = cpp_data.job_delivered.earned_xp;
    c_data.job_delivered.cargo_damage = cpp_data.job_delivered.cargo_damage;
    c_data.job_delivered.distance_km = cpp_data.job_delivered.distance_km;
    c_data.job_delivered.delivery_time = cpp_data.job_delivered.delivery_time;
    c_data.job_delivered.auto_park_used = cpp_data.job_delivered.auto_park_used;
    c_data.job_delivered.auto_load_used = cpp_data.job_delivered.auto_load_used;

    // Job Cancelled
    c_data.job_cancelled.penalty = cpp_data.job_cancelled.penalty;

    // Player Fined
    c_data.player_fined.fine_amount = cpp_data.player_fined.fine_amount;
    strcpy_s(c_data.player_fined.fine_offence, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.player_fined.fine_offence.c_str());

    // Tollgate Paid
    c_data.tollgate_paid.pay_amount = cpp_data.tollgate_paid.pay_amount;

    // Ferry Used
    c_data.ferry_used.pay_amount = cpp_data.ferry_used.pay_amount;
    strcpy_s(c_data.ferry_used.source_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.ferry_used.source_name.c_str());
    strcpy_s(c_data.ferry_used.target_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.ferry_used.target_name.c_str());
    strcpy_s(c_data.ferry_used.source_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.ferry_used.source_id.c_str());
    strcpy_s(c_data.ferry_used.target_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.ferry_used.target_id.c_str());

    // Train Used
    c_data.train_used.pay_amount = cpp_data.train_used.pay_amount;
    strcpy_s(c_data.train_used.source_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.train_used.source_name.c_str());
    strcpy_s(c_data.train_used.target_name, SPF_TELEMETRY_STRING_MAX_SIZE, cpp_data.train_used.target_name.c_str());
    strcpy_s(c_data.train_used.source_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.train_used.source_id.c_str());
    strcpy_s(c_data.train_used.target_id, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.train_used.target_id.c_str());

    callback(event_id, &c_data, user_data);
}

void TelemetryApi::InvokeGearboxConstantsCallback(const GearboxConstants& cpp_data, SPF_Telemetry_GearboxConstants_Callback callback, void* user_data) {
    SPF_GearboxConstants c_data;

    strcpy_s(c_data.shifter_type, SPF_TELEMETRY_ID_MAX_SIZE, cpp_data.shifter_type.c_str());

    c_data.slot_count = static_cast<uint32_t>(std::min<size_t>(cpp_data.slot_gear.size(), SPF_TELEMETRY_HSHIFTER_MAX_SLOTS));
    for (uint32_t i = 0; i < c_data.slot_count; ++i) {
        c_data.slot_gear[i] = cpp_data.slot_gear[i];
        c_data.slot_handle_position[i] = cpp_data.slot_handle_position[i];
        c_data.slot_selectors[i] = cpp_data.slot_selectors[i];
    }
    
    callback(&c_data, user_data);
}



void TelemetryApi::FillTelemetryApi(SPF_Telemetry_API* api) {
    if (!api) return;

    api->GetContext = &TelemetryApi::T_GetContext;
    api->GetGameState = &TelemetryApi::T_GetGameState;
    api->GetTimestamps = &TelemetryApi::T_GetTimestamps;
    api->GetCommonData = &TelemetryApi::T_GetCommonData;
    api->GetTruckConstants = &TelemetryApi::T_GetTruckConstants;
    api->GetTruckData = &TelemetryApi::T_GetTruckData;
    api->GetTrailers = &TelemetryApi::T_GetTrailers;
    api->GetJobConstants = &TelemetryApi::T_GetJobConstants;
    api->GetJobData = &TelemetryApi::T_GetJobData;
    api->GetNavigationData = &TelemetryApi::T_GetNavigationData;
    api->GetControls = &TelemetryApi::T_GetControls;
    api->GetSpecialEvents = &TelemetryApi::T_GetSpecialEvents;
    api->GetGameplayEvents = &TelemetryApi::T_GetGameplayEvents;
    api->GetGearboxConstants = &TelemetryApi::T_GetGearboxConstants;
    api->GetLastGameplayEventId = &TelemetryApi::T_GetLastGameplayEventId;

    // Assign new RAII-based event subscription functions
    api->RegisterForGameState = &TelemetryApi::T_RegisterForGameState;
    api->RegisterForTimestamps = &TelemetryApi::T_RegisterForTimestamps;
    api->RegisterForCommonData = &TelemetryApi::T_RegisterForCommonData;
    api->RegisterForTruckConstants = &TelemetryApi::T_RegisterForTruckConstants;
    api->RegisterForTrailerConstants = &TelemetryApi::T_RegisterForTrailerConstants;
    api->RegisterForTruckData = &TelemetryApi::T_RegisterForTruckData;
    api->RegisterForTrailers = &TelemetryApi::T_RegisterForTrailers;
    api->RegisterForJobConstants = &TelemetryApi::T_RegisterForJobConstants;
    api->RegisterForJobData = &TelemetryApi::T_RegisterForJobData;
    api->RegisterForNavigationData = &TelemetryApi::T_RegisterForNavigationData;
    api->RegisterForControls = &TelemetryApi::T_RegisterForControls;
    api->RegisterForSpecialEvents = &TelemetryApi::T_RegisterForSpecialEvents;
    api->RegisterForGameplayEvents = &TelemetryApi::T_RegisterForGameplayEvents;
    api->RegisterForGearboxConstants = &TelemetryApi::T_RegisterForGearboxConstants;


}

// --- Event Subscription (New RAII-based C-API Proxies) ---
SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForGameState(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameState_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::GameState>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::GameState& cpp_data, void* ud) {
        TelemetryApi::InvokeGameStateCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::GameState>>(
            pm.GetTelemetryService()->GetGameStateSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForTimestamps(SPF_Telemetry_Handle* handle, SPF_Telemetry_Timestamps_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::Timestamps>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::Timestamps& cpp_data, void* ud) {
        TelemetryApi::InvokeTimestampsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::Timestamps>>(
            pm.GetTelemetryService()->GetTimestampsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForCommonData(SPF_Telemetry_Handle* handle, SPF_Telemetry_CommonData_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::CommonData>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::CommonData& cpp_data, void* ud) {
        TelemetryApi::InvokeCommonDataCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::CommonData>>(
            pm.GetTelemetryService()->GetCommonDataSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForTruckConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckConstants_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::TruckConstants>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::TruckConstants& cpp_data, void* ud) {
        TelemetryApi::InvokeTruckConstantsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::TruckConstants>>(
            pm.GetTelemetryService()->GetTruckConstantsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForTrailerConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_TrailerConstants_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::TrailerConstants>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::TrailerConstants& cpp_data, void* ud) {
        TelemetryApi::InvokeTrailerConstantsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::TrailerConstants>>(
            pm.GetTelemetryService()->GetTrailerConstantsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForTruckData(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckData_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::TruckData>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::TruckData& cpp_data, void* ud) {
        TelemetryApi::InvokeTruckDataCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::TruckData>>(
            pm.GetTelemetryService()->GetTruckDataSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForTrailers(SPF_Telemetry_Handle* handle, SPF_Telemetry_Trailers_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<std::vector<SPF::Telemetry::SCS::Trailer>>::InvokerFunction invoker =
        [callback](const std::vector<SPF::Telemetry::SCS::Trailer>& cpp_data, void* ud) {
        TelemetryApi::InvokeTrailersCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<std::vector<SPF::Telemetry::SCS::Trailer>>>(
            pm.GetTelemetryService()->GetTrailersSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForJobConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobConstants_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::JobConstants>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::JobConstants& cpp_data, void* ud) {
        TelemetryApi::InvokeJobConstantsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::JobConstants>>(
            pm.GetTelemetryService()->GetJobConstantsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForJobData(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobData_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::JobData>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::JobData& cpp_data, void* ud) {
        TelemetryApi::InvokeJobDataCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::JobData>>(
            pm.GetTelemetryService()->GetJobDataSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForNavigationData(SPF_Telemetry_Handle* handle, SPF_Telemetry_NavigationData_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::NavigationData>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::NavigationData& cpp_data, void* ud) {
        TelemetryApi::InvokeNavigationDataCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::NavigationData>>(
            pm.GetTelemetryService()->GetNavigationDataSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForControls(SPF_Telemetry_Handle* handle, SPF_Telemetry_Controls_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::Controls>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::Controls& cpp_data, void* ud) {
        TelemetryApi::InvokeControlsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::Controls>>(
            pm.GetTelemetryService()->GetControlsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForSpecialEvents(SPF_Telemetry_Handle* handle, SPF_Telemetry_SpecialEvents_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::SpecialEvents>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::SpecialEvents& cpp_data, void* ud) {
        TelemetryApi::InvokeSpecialEventsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::SpecialEvents>>(
            pm.GetTelemetryService()->GetSpecialEventsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForGameplayEvents(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameplayEvents_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    GameplayEventSubscriptionHandler::InvokerFunction invoker =
        [callback](const char* event_id, const SPF::Telemetry::SCS::GameplayEvents& cpp_data, void* ud) {
        TelemetryApi::InvokeGameplayEventsCallback(event_id, cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<GameplayEventSubscriptionHandler>(
            pm.GetTelemetryService()->GetGameplayEventsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}

SPF_Telemetry_Callback_Handle* TelemetryApi::T_RegisterForGearboxConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_GearboxConstants_Callback callback, void* user_data) {
    auto& pm = PluginManager::GetInstance();
    if (!handle || !callback || !pm.GetTelemetryService()) return nullptr;

    Handles::TelemetryHandle* telemetryHandle = reinterpret_cast<Handles::TelemetryHandle*>(handle);
    if (!telemetryHandle) {
        return nullptr;
    }

    SubscriptionHandler<SPF::Telemetry::SCS::GearboxConstants>::InvokerFunction invoker =
        [callback](const SPF::Telemetry::SCS::GearboxConstants& cpp_data, void* ud) {
        TelemetryApi::InvokeGearboxConstantsCallback(cpp_data, callback, ud);
    };

    telemetryHandle->m_subscriptionHandlers.emplace_back(
        std::make_unique<SubscriptionHandler<SPF::Telemetry::SCS::GearboxConstants>>(
            pm.GetTelemetryService()->GetGearboxConstantsSignal(),
            invoker,
            user_data
        )
    );
    return reinterpret_cast<SPF_Telemetry_Callback_Handle*>(telemetryHandle->m_subscriptionHandlers.back().get());
}


} // namespace Modules::API
SPF_NS_END
