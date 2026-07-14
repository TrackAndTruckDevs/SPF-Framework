#pragma once

/**
 * @file Sdk.hpp
 * @brief Centralized include point for all SCS SDK headers.
 *
 * This file includes all necessary headers from the SCS Telemetry SDK in
 * the correct, defined order to prevent compilation errors.
 * All other framework files that need to interact with the SDK should
 * include this file, not individual SDK headers.
 */

// Basic SDK definitions
#include <scssdk.h>        // IWYU pragma: export
#include <scssdk_value.h>  // IWYU pragma: export

// Common telemetry definitions
#include <common/scssdk_telemetry_common_channels.h>          // IWYU pragma: export
#include <common/scssdk_telemetry_common_configs.h>           // IWYU pragma: export
#include <common/scssdk_telemetry_common_gameplay_events.h>   // IWYU pragma: export
#include <common/scssdk_telemetry_job_common_channels.h>      // IWYU pragma: export
#include <common/scssdk_telemetry_trailer_common_channels.h>  // IWYU pragma: export
#include <common/scssdk_telemetry_truck_common_channels.h>    // IWYU pragma: export
#include <scssdk_telemetry_channel.h>                         // IWYU pragma: export
#include <scssdk_telemetry_event.h>                           // IWYU pragma: export


// Specific definitions for Euro Truck Simulator 2
#include <eurotrucks2/scssdk_eut2.h>            // IWYU pragma: export
#include <eurotrucks2/scssdk_input_eut2.h>      // IWYU pragma: export
#include <eurotrucks2/scssdk_telemetry_eut2.h>  // IWYU pragma: export

// Specific definitions for American Truck Simulator
#include <amtrucks/scssdk_ats.h>            // IWYU pragma: export
#include <amtrucks/scssdk_input_ats.h>      // IWYU pragma: export
#include <amtrucks/scssdk_telemetry_ats.h>  // IWYU pragma: export
#include <scssdk_input.h>                   // IWYU pragma: export
#include <scssdk_input_device.h>            // IWYU pragma: export
#include <scssdk_input_event.h>             // IWYU pragma: export


// Main telemetry header
#include <scssdk_telemetry.h>  // IWYU pragma: export
