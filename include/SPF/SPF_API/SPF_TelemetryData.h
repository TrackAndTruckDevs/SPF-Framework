#pragma once

#include <stdint.h>
#include <stdbool.h>

// This file defines the C-compatible data structures for the Telemetry API.
// It is designed to be included by plugins that wish to access telemetry data.
// The layout and types are fixed to ensure ABI stability.

// --- Max Sizes for Arrays ---
// These values are based on the SCS SDK headers (e.g., scssdk_telemetry_common.h)
#define SPF_TELEMETRY_SUBSTANCE_MAX_COUNT 64
#define SPF_TELEMETRY_CONTROLS_MAX_COUNT 32
#define SPF_TELEMETRY_WHEEL_MAX_COUNT 32
#define SPF_TELEMETRY_TRAILER_MAX_COUNT 10
#define SPF_TELEMETRY_GEAR_MAX_COUNT 32
#define SPF_TELEMETRY_HSHIFTER_MAX_SLOTS 32
#define SPF_TELEMETRY_SELECTOR_MAX_COUNT 8
#define SPF_TELEMETRY_ID_MAX_SIZE 64
#define SPF_TELEMETRY_STRING_MAX_SIZE 256


// --- Forward Declarations for nested structs ---
typedef struct SPF_Placement SPF_Placement;
typedef struct SPF_DPlacement SPF_DPlacement;
typedef struct SPF_FVector SPF_FVector;
typedef struct SPF_DVector SPF_DVector;
typedef struct SPF_Euler SPF_Euler;
typedef struct SPF_DEuler SPF_DEuler;


// --- Basic Vector/Placement Types ---

/**
 * @struct SPF_FVector
 * @brief Represents a 3D vector with single-precision floating-point components.
 *        Used for positions, velocities, and accelerations where high precision is not critical.
 */
struct SPF_FVector {
    float x; ///< The x-component of the vector.
    float y; ///< The y-component of the vector.
    float z; ///< The z-component of the vector.
};

/**
 * @struct SPF_DVector
 * @brief Represents a 3D vector with double-precision floating-point components.
 *        Used for high-precision world coordinates.
 */
struct SPF_DVector {
    double x; ///< The x-component of the vector.
    double y; ///< The y-component of the vector.
    double z; ///< The z-component of the vector.
};

/**
 * @struct SPF_Euler
 * @brief Represents orientation in 3D space using Euler angles (heading, pitch, roll)
 *        with single-precision floating-point values.
 */
struct SPF_Euler {
    float heading; ///< Rotation around the vertical (Y) axis.
    float pitch;   ///< Rotation around the transverse (X) axis.
    float roll;    ///< Rotation around the longitudinal (Z) axis.
};

/**
 * @struct SPF_DEuler
 * @brief Represents orientation in 3D space using Euler angles with double-precision
 *        floating-point values for higher accuracy.
 */
struct SPF_DEuler {
    double heading; ///< Rotation around the vertical (Y) axis.
    double pitch;   ///< Rotation around the transverse (X) axis.
    double roll;    ///< Rotation around the longitudinal (Z) axis.
};

/**
 * @struct SPF_Placement
 * @brief Represents the position and orientation of an object in 3D space using
 *        single-precision floats.
 */
struct SPF_Placement {
    SPF_FVector position;    ///< The 3D position of the object.
    SPF_Euler orientation; ///< The Euler angle orientation of the object.
};

/**
 * @struct SPF_DPlacement
 * @brief Represents the position and orientation of an object in 3D space with
 *        double-precision for high-accuracy world coordinates.
 */
struct SPF_DPlacement {
    SPF_DVector position;    ///< The high-precision 3D position of the object.
    SPF_DEuler orientation; ///< The high-precision Euler angle orientation of the object.
};


// --- Main Data Structures ---

typedef struct {
    /**
     * @brief Internal ID of the game currently running.
     *
     * Possible values as defined by the SCS SDK:
     * - "ets2" (Euro Truck Simulator 2)
     * - "ats" (American Truck Simulator)
     * - "unknown" (When game is not identified)
     */
    char game_id[SPF_TELEMETRY_ID_MAX_SIZE];
    char game_name[SPF_TELEMETRY_STRING_MAX_SIZE];
    uint32_t scs_game_version_major;
    uint32_t scs_game_version_minor;
    uint32_t telemetry_plugin_version_major;
    uint32_t telemetry_plugin_version_minor;
    uint32_t telemetry_game_version_major;
    uint32_t telemetry_game_version_minor;
    bool paused;
    float scale;
    int32_t multiplayer_time_offset;
} SPF_GameState;

typedef struct {
    uint64_t simulation;
    uint64_t render;
    uint64_t paused_simulation;
} SPF_Timestamps;

/**
 * @struct SPF_CommonData
 * @brief Contains common, frequently updated telemetry data.
 */
typedef struct {
    /**
     * @brief Current in-game time.
     * @unit minutes
     * @source scs_telemetry_common_channels.h
     *
     * Represents the number of minutes passed in the game since the start of the current session.
     */
    uint32_t game_time;

    /**
     * @brief Time until the next rest stop is required.
     * @unit minutes
     *
     * A positive value indicates time remaining. A negative value indicates time overdue.
     */
    int32_t next_rest_stop;

    /**
     * @struct next_rest_stop_time
     * @brief Calculated in-game time for the next rest stop.
     * This is not from the SDK but calculated by SPF for convenience.
     */
    struct {
        uint32_t DayOfWeek; ///< Calculated day of the week for the next rest stop (1=Monday, 7=Sunday).
        uint32_t Hour;      ///< Calculated hour for the next rest stop (0-23).
        uint32_t Minute;    ///< Calculated minute for the next rest stop (0-59).
    } next_rest_stop_time;

    /**
     * @brief Time until the next rest stop in real-world minutes.
     * This is not from the SDK but calculated by SPF for convenience.
     */
    float next_rest_stop_real_minutes;

    /**
     * @brief An array of identifiers for all substances (surface types) known to the game.
     * The index of a wheel's current substance can be used to look up the name in this array.
     */
    char substances[SPF_TELEMETRY_SUBSTANCE_MAX_COUNT][SPF_TELEMETRY_ID_MAX_SIZE];

    /**
     * @brief The number of valid substance identifiers in the `substances` array.
     */
    uint32_t substance_count;
} SPF_CommonData;

/**
 * @struct SPF_ControlInput
 * @brief Represents a set of control inputs, normalized from 0.0 to 1.0.
 */
typedef struct {
    float steering; ///< Steering input. Ranges from -1.0 (full left) to 1.0 (full right).
    float throttle; ///< Throttle input.
    float brake;    ///< Brake input.
    float clutch;   ///< Clutch input.
} SPF_ControlInput;

/**
 * @struct SPF_Controls
 * @brief Holds both the raw user input and the final, effective input after game processing.
 */
typedef struct {
    SPF_ControlInput userInput;      ///< Raw input from the user's hardware (e.g., steering wheel, pedals).
    SPF_ControlInput effectiveInput; ///< The final input applied in the simulation after assists and game logic.
} SPF_Controls;

/**
 * @struct SPF_WheelConstants
 * @brief Contains static, unchanging properties for a single wheel.
 */
typedef struct {
    bool simulated;  ///< Is this wheel simulated?
    bool powered;    ///< Is this wheel powered by the engine?
    bool steerable;  ///< Can this wheel be steered?
    bool liftable;   ///< Is this wheel part of a liftable axle?
    float radius;    ///< The radius of the wheel. @unit meters
    SPF_FVector position; ///< The 3D position of the wheel relative to the vehicle's origin.
} SPF_WheelConstants;

/**
 * @struct SPF_WheelData
 * @brief Contains dynamic, frequently changing data for a single wheel.
 */
typedef struct {
    float suspension_deflection; ///< Current vertical deflection of the suspension. @unit meters
    bool on_ground;              ///< Is the wheel currently touching the ground?
    uint32_t substance;          ///< Index of the surface substance the wheel is on. See `SPF_CommonData.substances`.
    float angular_velocity;      ///< The wheel's angular velocity. @unit radians/second
    float steering;              ///< The current steering angle of the wheel. @unit radians
    float rotation;              ///< The wheel's current rotation angle. @unit radians
    float lift;                  ///< The current lift of the axle. 0.0 is fully on the ground, 1.0 is fully lifted.
    float lift_offset;           ///< The suspension offset caused by the axle lift.
} SPF_WheelData;

/**
 * @struct SPF_TruckConstants
 * @brief Contains static, unchanging properties of the player's truck.
 *        This data describes the truck's configuration and does not change during gameplay,
 *        unless the player changes their truck at a garage.
 */
typedef struct {
    // --- Identification ---
    char brand_id[SPF_TELEMETRY_ID_MAX_SIZE];   ///< Internal ID of the truck's brand (e.g., "scania").
    char brand[SPF_TELEMETRY_STRING_MAX_SIZE];  ///< Display name of the truck's brand (e.g., "Scania").
    char id[SPF_TELEMETRY_ID_MAX_SIZE];         ///< Internal ID of the truck model (e.g., "s_2016").
    char name[SPF_TELEMETRY_STRING_MAX_SIZE];   ///< Display name of the truck model (e.g., "S 730").
    char license_plate[SPF_TELEMETRY_ID_MAX_SIZE]; ///< The license plate text.
    char license_plate_country_id[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Internal ID for the license plate's country.
    char license_plate_country[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name for the license plate's country.

    // --- Capacities & Warnings ---
    float fuel_capacity;           ///< Maximum fuel tank capacity. @unit liters
    float fuel_warning_factor;     ///< Fuel level percentage at which the low fuel warning activates.
    float adblue_capacity;         ///< Maximum AdBlue tank capacity. @unit liters
    float adblue_warning_factor;   ///< AdBlue level percentage at which the low AdBlue warning activates.

    // --- Pressure & Temperature Warnings ---
    float air_pressure_warning;    ///< Air pressure level at which the warning activates. @unit psi
    float air_pressure_emergency;  ///< Air pressure level at which the emergency warning activates. @unit psi
    float oil_pressure_warning;    ///< Oil pressure level at which the warning activates. @unit psi
    float water_temperature_warning; ///< Water temperature at which the warning activates. @unit celsius
    float battery_voltage_warning; ///< Battery voltage at which the warning activates. @unit volts

    // --- Drivetrain ---
    float rpm_limit;               ///< The engine's RPM limit.
    uint32_t forward_gear_count;   ///< Number of forward gears.
    uint32_t reverse_gear_count;   ///< Number of reverse gears.
    uint32_t retarder_step_count;  ///< Number of steps for the retarder brake.
    uint32_t selector_count;       ///< Number of H-shifter selectors.
    float differential_ratio;      ///< The differential gear ratio.

    // --- Positions ---
    SPF_FVector cabin_position;    ///< Position of the cabin's pivot point relative to the truck's origin.
    SPF_FVector head_position;     ///< Position of the driver's head relative to the cabin's pivot point.
    SPF_FVector hook_position;     ///< Position of the trailer hook-up point relative to the truck's origin.

    // --- Wheels ---
    SPF_WheelConstants wheels[SPF_TELEMETRY_WHEEL_MAX_COUNT]; ///< Array of wheel constant data.
    uint32_t wheel_count;          ///< Number of wheels on the truck.

    // --- Gear Ratios ---
    float gear_ratios_forward[SPF_TELEMETRY_GEAR_MAX_COUNT]; ///< Array of forward gear ratios.
    float gear_ratios_reverse[SPF_TELEMETRY_GEAR_MAX_COUNT]; ///< Array of reverse gear ratios.

} SPF_TruckConstants;

/**
 * @struct SPF_TruckData
 * @brief Contains dynamic, frequently changing data about the player's truck.
 */
typedef struct {
    // --- Physics ---
    SPF_DPlacement world_placement; ///< High-precision world position and orientation of the truck.
    SPF_FVector local_linear_velocity;    ///< Velocity vector in local space. @unit meters/second
    SPF_FVector local_angular_velocity;   ///< Angular velocity vector in local space. @unit radians/second
    SPF_FVector local_linear_acceleration;  ///< Linear acceleration vector in local space. @unit meters/second^2
    SPF_FVector local_angular_acceleration; ///< Angular acceleration vector in local space. @unit radians/second^2

    // --- Cabin & Head ---
    SPF_Placement cabin_offset; ///< Position and orientation of the cabin relative to the chassis.
    SPF_FVector cabin_angular_velocity;     ///< Angular velocity of the cabin.
    SPF_FVector cabin_angular_acceleration; ///< Angular acceleration of the cabin.
    SPF_Placement head_offset;    ///< Position and orientation of the driver's head relative to the cabin.

    // --- Drivetrain ---
    float speed;           ///< Speed of the truck. @unit meters/second
    float engine_rpm;      ///< Current engine RPM.
    int32_t gear;          ///< Currently selected gear (0=neutral, >0=forward, <0=reverse).
    int32_t displayed_gear;///< The gear displayed on the dashboard.

    // --- Inputs ---
    float input_steering; ///< Raw steering input from the user.
    float input_throttle; ///< Raw throttle input from the user.
    float input_brake;    ///< Raw brake input from the user.
    float input_clutch;   ///< Raw clutch input from the user.

    float effective_steering; ///< Effective steering value after game processing.
    float effective_throttle; ///< Effective throttle value after game processing.
    float effective_brake;    ///< Effective brake value after game processing.
    float effective_clutch;   ///< Effective clutch value after game processing.

    // --- Cruise Control & Shifter ---
    float cruise_control_speed; ///< Cruise control speed setting. @unit meters/second
    uint32_t hshifter_slot;     ///< The physical H-shifter slot the stick is in (0=neutral).
    bool hshifter_selector[SPF_TELEMETRY_SELECTOR_MAX_COUNT]; ///< State of the H-shifter selectors/toggles.

    // --- Brakes ---
    bool parking_brake;  ///< Is the parking brake engaged?
    bool motor_brake;    ///< Is the motor brake (engine brake) engaged?
    uint32_t retarder_level; ///< Current retarder brake level (0 for off).

    // --- Pressures & Temperatures ---
    float air_pressure;       ///< Air pressure in the brake system. @unit psi
    bool air_pressure_warning;   ///< Is the air pressure warning active?
    bool air_pressure_emergency; ///< Is the air pressure emergency active?
    float brake_temperature;  ///< Temperature of the brakes. @unit celsius

    // --- Fuel & AdBlue ---
    float fuel_amount;              ///< Current amount of fuel. @unit liters
    bool fuel_warning;              ///< Is the low fuel warning active?
    float fuel_average_consumption; ///< Average fuel consumption. @unit liters/km
    float fuel_range;               ///< Estimated range with current fuel. @unit km

    float adblue_amount;            ///< Current amount of AdBlue. @unit liters
    bool adblue_warning;            ///< Is the low AdBlue warning active?
    float adblue_average_consumption; ///< Average AdBlue consumption.

    // --- Engine & Electrics ---
    float oil_pressure;          ///< Oil pressure. @unit psi
    bool oil_pressure_warning;   ///< Is the oil pressure warning active?
    float oil_temperature;       ///< Oil temperature. @unit celsius
    float water_temperature;     ///< Water temperature. @unit celsius
    bool water_temperature_warning; ///< Is the water temperature warning active?
    float battery_voltage;       ///< Battery voltage. @unit volts
    bool battery_voltage_warning; ///< Is the battery voltage warning active?

    bool electric_enabled; ///< Are the truck's electronics enabled?
    bool engine_enabled;   ///< Is the engine running?
    bool wipers;           ///< Are the wipers active?

    // --- Axles ---
    bool differential_lock; ///< Is the differential lock engaged?
    bool lift_axle;         ///< Is the truck's liftable axle raised?
    bool lift_axle_indicator; ///< State of the lift axle indicator on the dashboard.
    bool trailer_lift_axle; ///< Is the trailer's liftable axle raised?
    bool trailer_lift_axle_indicator; ///< State of the trailer's lift axle indicator.

    // --- Lights ---
    bool lblinker;     ///< Is the left blinker on?
    bool rblinker;     ///< Is the right blinker on?
    bool hazard_warning; ///< Are the hazard lights on?
    bool light_lblinker; ///< Is the left blinker light on the dashboard active?
    bool light_rblinker; ///< Is the right blinker light on the dashboard active?
    bool light_parking;  ///< Are the parking lights on?
    bool light_low_beam; ///< Are the low beam headlights on?
    bool light_high_beam;///< Are the high beam headlights on?
    uint32_t light_aux_front; ///< State of the front auxiliary lights (bitmask).
    uint32_t light_aux_roof;  ///< State of the roof auxiliary lights (bitmask).
    bool light_beacon;   ///< Are the beacon lights on?
    bool light_brake;    ///< Are the brake lights on?
    bool light_reverse;  ///< Are the reverse lights on?
    float dashboard_backlight; ///< Backlight intensity of the dashboard.

    // --- Damage & Odometer ---
    float wear_engine;       ///< Engine wear level (0.0-1.0).
    float wear_transmission; ///< Transmission wear level (0.0-1.0).
    float wear_cabin;        ///< Cabin wear level (0.0-1.0).
    float wear_chassis;      ///< Chassis wear level (0.0-1.0).
    float wear_wheels;       ///< Average wear level of all wheels (0.0-1.0).

    float odometer; ///< The truck's odometer reading. @unit km

    // --- Wheels ---
    SPF_WheelData wheels[SPF_TELEMETRY_WHEEL_MAX_COUNT]; ///< Array of dynamic wheel data.

} SPF_TruckData;

/**
 * @struct SPF_TrailerConstants
 * @brief Contains static, unchanging properties of a trailer.
 */
typedef struct {
    // --- Identification ---
    char id[SPF_TELEMETRY_ID_MAX_SIZE];         ///< Internal ID of the trailer model.
    char cargo_accessory_id[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Internal ID of the cargo accessory.
    char brand_id[SPF_TELEMETRY_ID_MAX_SIZE];   ///< Internal ID of the trailer's brand.
    char brand[SPF_TELEMETRY_STRING_MAX_SIZE];  ///< Display name of the trailer's brand.
    char name[SPF_TELEMETRY_STRING_MAX_SIZE];   ///< Display name of the trailer model.
    char chain_type[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Type of trailer chain (e.g., single, double).
    char body_type[SPF_TELEMETRY_ID_MAX_SIZE];  ///< The body type of the trailer (e.g., "curtainside").
    char license_plate[SPF_TELEMETRY_ID_MAX_SIZE]; ///< The license plate text.
    char license_plate_country_id[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Internal ID for the license plate's country.
    char license_plate_country[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name for the license plate's country.

    // --- Positions & Wheels ---
    SPF_FVector hook_position;     ///< Position of the hook point relative to the trailer's origin.
    uint32_t wheel_count;          ///< Number of wheels on the trailer.
    SPF_WheelConstants wheels[SPF_TELEMETRY_WHEEL_MAX_COUNT]; ///< Array of wheel constant data.
} SPF_TrailerConstants;

/**
 * @struct SPF_TrailerData
 * @brief Contains dynamic, frequently changing data for a trailer.
 */
typedef struct {
    bool connected;              ///< Is the trailer currently connected to the truck?
    float cargo_damage;          ///< Current cargo damage level (0.0-1.0).

    // --- Physics ---
    SPF_DPlacement world_placement; ///< High-precision world position and orientation of the trailer.
    SPF_FVector local_linear_velocity;    ///< Velocity vector in local space.
    SPF_FVector local_angular_velocity;   ///< Angular velocity vector in local space.
    SPF_FVector local_linear_acceleration;  ///< Linear acceleration vector in local space.
    SPF_FVector local_angular_acceleration; ///< Angular acceleration vector in local space.

    // --- Wear ---
    float wear_body;    ///< Body wear level (0.0-1.0).
    float wear_chassis; ///< Chassis wear level (0.0-1.0).
    float wear_wheels;  ///< Average wear level of all wheels (0.0-1.0).

    // --- Wheels ---
    SPF_WheelData wheels[SPF_TELEMETRY_WHEEL_MAX_COUNT]; ///< Array of dynamic wheel data.
} SPF_TrailerData;

/**
 * @struct SPF_Trailer
 * @brief A container struct that groups the constant and dynamic data for a single trailer.
 * The game supports multiple trailers, which will be provided as an array of this struct.
 */
typedef struct {
    SPF_TrailerConstants constants; ///< Static properties of the trailer.
    SPF_TrailerData data;           ///< Dynamic data for the trailer.
} SPF_Trailer;

/**
 * @struct SPF_JobConstants
 * @brief Contains static information about the current job. This data does not change during the job.
 */
typedef struct {
    // --- Job Details ---
    uint64_t income;                ///< The total income for completing the job.
    uint32_t delivery_time;         ///< The allotted time for the delivery. @unit minutes
    uint32_t planned_distance_km;   ///< The planned distance for the job. @unit km
    bool is_cargo_loaded;           ///< Is the cargo currently loaded?
    bool is_special_job;            ///< Is this a special job (e.g., World of Trucks)?

    /**
     * @brief The market the job was taken from.
     *
     * Possible values as defined by the SCS SDK are:
     * - "cargo_market"
     * - "quick_job"
     * - "freight_market"
     * - "external_contracts"
     * - "external_market"
     */
    char job_market[SPF_TELEMETRY_ID_MAX_SIZE];

    // --- Cargo Details ---
    char cargo_id[SPF_TELEMETRY_ID_MAX_SIZE];     ///< Internal ID of the cargo type.
    char cargo_name[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name of the cargo.
    float cargo_mass;               ///< Total mass of the cargo. @unit kilograms
    uint32_t cargo_unit_count;      ///< For cargoes composed of multiple units (e.g., pallets).
    float cargo_unit_mass;          ///< Mass of a single cargo unit. @unit kilograms

    // --- Destination ---
    char destination_city_id[SPF_TELEMETRY_ID_MAX_SIZE];    ///< Internal ID of the destination city.
    char destination_city[SPF_TELEMETRY_STRING_MAX_SIZE];   ///< Display name of the destination city.
    char destination_company_id[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Internal ID of the destination company.
    char destination_company[SPF_TELEMETRY_STRING_MAX_SIZE];///< Display name of the destination company.

    // --- Source ---
    char source_city_id[SPF_TELEMETRY_ID_MAX_SIZE];    ///< Internal ID of the source city.
    char source_city[SPF_TELEMETRY_STRING_MAX_SIZE];   ///< Display name of the source city.
    char source_company_id[SPF_TELEMETRY_ID_MAX_SIZE]; ///< Internal ID of the source company.
    char source_company[SPF_TELEMETRY_STRING_MAX_SIZE];///< Display name of the source company.
} SPF_JobConstants;

/**
 * @struct SPF_JobData
 * @brief Contains dynamic, frequently changing data about the current job.
 */
typedef struct {
    bool on_job;                     ///< Is the player currently on a job?
    float cargo_damage;              ///< Current damage to the cargo (0.0-1.0).
    uint32_t remaining_delivery_minutes; ///< Remaining time for the delivery. @unit minutes
} SPF_JobData;

/**
 * @struct SPF_NavigationData
 * @brief Contains data related to the in-game GPS navigation.
 */
typedef struct {
    float navigation_distance;      ///< Remaining distance to the destination. @unit meters
    float navigation_time;          ///< Estimated time to destination. @unit seconds
    float navigation_speed_limit;   ///< Current speed limit on the road. @unit meters/second
    float navigation_time_real_seconds; ///< Estimated time to destination. @unit real-world seconds
} SPF_NavigationData;

/**
 * @struct SPF_SpecialEvents
 * @brief Contains flags for one-time events that have occurred. These flags are typically true
 *        for a single telemetry frame and then reset.
 */
typedef struct {
    bool job_delivered; ///< True for one frame when a job is delivered.
    bool job_cancelled; ///< True for one frame when a job is cancelled.
    bool fined;         ///< True for one frame when the player is fined.
    bool tollgate;      ///< True for one frame when the player pays a toll.
    bool ferry;         ///< True for one frame when the player uses a ferry.
    bool train;         ///< True for one frame when the player uses a train.
} SPF_SpecialEvents;

/**
 * @struct SPF_GameplayEvent_JobDelivered
 * @brief Data associated with a 'job_delivered' event.
 */
typedef struct {
    int64_t revenue;          ///< The final revenue for the job.
    int32_t earned_xp;        ///< Experience points earned.
    float cargo_damage;       ///< Final cargo damage (0.0-1.0).
    float distance_km;        ///< The actual distance driven for the job. @unit km
    uint32_t delivery_time;   ///< The time taken to complete the job. @unit minutes
    bool auto_park_used;      ///< Was auto-parking used?
    bool auto_load_used;      ///< Was auto-loading used?
} SPF_GameplayEvent_JobDelivered;

/**
 * @struct SPF_GameplayEvent_JobCancelled
 * @brief Data associated with a 'job_cancelled' event.
 */
typedef struct {
    int64_t penalty; ///< The financial penalty for cancelling the job.
} SPF_GameplayEvent_JobCancelled;

/**
 * @struct SPF_GameplayEvent_PlayerFined
 * @brief Data associated with a 'player.fined' event.
 */
typedef struct {
    /** @brief The amount of the fine, in the game's native currency. */
    int64_t fine_amount;

    /**
     * @brief A string identifier for the offence.
     *
     * Possible values as defined by the SCS SDK are:
     * - "crash"
     * - "avoid_sleeping"
     * - "wrong_way"
     * - "speeding_camera"
     * - "no_lights"
     * - "red_signal"
     * - "speeding"
     * - "avoid_weighing"
     * - "illegal_trailer"
     * - "avoid_inspection"
     * - "illegal_border_crossing"
     * - "hard_shoulder_violation"
     * - "damaged_vehicle_usage"
     * - "generic"
     */
    char fine_offence[SPF_TELEMETRY_ID_MAX_SIZE];
} SPF_GameplayEvent_PlayerFined;

/**
 * @struct SPF_GameplayEvent_TollgatePaid
 * @brief Data associated with a 'tollgate' event.
 */
typedef struct {
    int64_t pay_amount; ///< The amount paid at the tollgate.
} SPF_GameplayEvent_TollgatePaid;

/**
 * @struct SPF_GameplayEvent_FerryUsed
 * @brief Data associated with a 'ferry' event.
 */
typedef struct {
    int64_t pay_amount;  ///< The amount paid for the ferry.
    char source_name[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name of the source port.
    char target_name[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name of the target port.
    char source_id[SPF_TELEMETRY_ID_MAX_SIZE];       ///< Internal ID of the source port.
    char target_id[SPF_TELEMETRY_ID_MAX_SIZE];       ///< Internal ID of the target port.
} SPF_GameplayEvent_FerryUsed;

/**
 * @struct SPF_GameplayEvent_TrainUsed
 * @brief Data associated with a 'train' event.
 */
typedef struct {
    int64_t pay_amount;  ///< The amount paid for the train.
    char source_name[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name of the source station.
    char target_name[SPF_TELEMETRY_STRING_MAX_SIZE]; ///< Display name of the target station.
    char source_id[SPF_TELEMETRY_ID_MAX_SIZE];       ///< Internal ID of the source station.
    char target_id[SPF_TELEMETRY_ID_MAX_SIZE];       ///< Internal ID of the target station.
} SPF_GameplayEvent_TrainUsed;

/**
 * @struct SPF_GameplayEvents
 * @brief A container for all gameplay event data structures. When a special event flag is true,
 *        the corresponding struct in this container will be filled with data for that frame.
 */
typedef struct {
    SPF_GameplayEvent_JobDelivered job_delivered;
    SPF_GameplayEvent_JobCancelled job_cancelled;
    SPF_GameplayEvent_PlayerFined player_fined;
    SPF_GameplayEvent_TollgatePaid tollgate_paid;
    SPF_GameplayEvent_FerryUsed ferry_used;
    SPF_GameplayEvent_TrainUsed train_used;
} SPF_GameplayEvents;

/**
 * @struct SPF_GearboxConstants
 * @brief Contains static information about the truck's H-shifter gearbox layout.
 */
typedef struct {
    /**
     * @brief The type of shifter installed in the truck.
     *
     * Possible values as defined by the SCS SDK:
     * - "arcade"
     * - "automatic"
     * - "manual"
     * - "hshifter"
     */
    char shifter_type[SPF_TELEMETRY_ID_MAX_SIZE];

    /**
     * @brief Maps a physical H-shifter slot index to a game gear.
     * The index of the array corresponds to the `hshifter_slot` value from `SPF_TruckData`.
     * The value at that index is the gear that slot is mapped to.
     */
    int32_t slot_gear[SPF_TELEMETRY_HSHIFTER_MAX_SLOTS];

    /**
     * @brief The handle position index for each slot.
     * This is used internally by the game and may not be useful for most plugins.
     */
    uint32_t slot_handle_position[SPF_TELEMETRY_HSHIFTER_MAX_SLOTS];

    /**
     * @brief Bitmask indicating which selectors are active for each slot.
     */
    uint32_t slot_selectors[SPF_TELEMETRY_HSHIFTER_MAX_SLOTS];

    ///< The total number of configured slots in the H-shifter layout.
    uint32_t slot_count;
} SPF_GearboxConstants;
