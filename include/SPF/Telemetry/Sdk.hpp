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
#include <scssdk.h>
#include <scssdk_value.h>

// Common telemetry definitions
#include <common/scssdk_telemetry_common_channels.h>
#include <common/scssdk_telemetry_common_configs.h>
#include <common/scssdk_telemetry_common_gameplay_events.h>
#include <common/scssdk_telemetry_job_common_channels.h>
#include <common/scssdk_telemetry_trailer_common_channels.h>
#include <common/scssdk_telemetry_truck_common_channels.h>
#include <scssdk_telemetry_event.h>
#include <scssdk_telemetry_channel.h>

// Specific definitions for Euro Truck Simulator 2
#include <eurotrucks2/scssdk_eut2.h>
#include <eurotrucks2/scssdk_input_eut2.h>
#include <eurotrucks2/scssdk_telemetry_eut2.h>

// Specific definitions for American Truck Simulator
#include <amtrucks/scssdk_ats.h>
#include <amtrucks/scssdk_input_ats.h>
#include <amtrucks/scssdk_telemetry_ats.h>

#include <scssdk_input.h>
#include <scssdk_input_device.h>
#include <scssdk_input_event.h>

// Main telemetry header
#include <scssdk_telemetry.h>
