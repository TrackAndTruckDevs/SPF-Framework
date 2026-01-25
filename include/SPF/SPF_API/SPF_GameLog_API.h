/**                                                                                               
* @file SPF_GameLog_API.h                                                                          
* @brief API for subscribing to and monitoring the game's internal log output.
*                                                                                                 
* @details This API provides a real-time stream of every message written to the 
* game's log file ('game.log.txt'). It is a powerful tool for monitoring game 
* events that don't have dedicated telemetry properties, such as economic 
* transactions, asset loading notifications, or scripted events.
*                                                                                                 
* Plugins can register high-performance callbacks to filter and react to specific 
* log patterns as they occur.                                       
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **High Frequency**: Game logs can be extremely verbose. Ensure your callback 
*    logic is fast and non-blocking to prevent performance degradation.
*                                                                                                 
* 2. **Automatic Lifecycle**: Subscription handles ('SPF_GameLog_Callback_Handle') are 
*    managed by the framework. When your main context handle is destroyed during 
*    plugin shutdown, all associated log subscriptions are automatically cleaned up.
*                                                                                                 
* 3. **Pattern Matching**: Since log lines are raw strings, using functions like 
*    'strstr' or regex is the standard way to detect specific events.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* void OnGameLog(const char* message, void* userData) {
*     if (strstr(message, "Economy local time")) {
*         // React to time change event found in log
*     }
* }
*
* void OnActivated(const SPF_Core_API* api) {
*     SPF_GameLog_Handle* h = api->gamelog->GLog_GetContext("MyPlugin");
*     api->gamelog->GLog_RegisterCallback(h, OnGameLog, nullptr);
* }
* @endcode                                                                                        
*/ 

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// Types
// =================================================================================================

/**
 * @brief Opaque handle to the GameLog API context for a specific plugin.
 */
typedef struct SPF_GameLog_Handle SPF_GameLog_Handle;

/**
 * @brief Opaque handle representing a specific log subscription.
 */
typedef struct SPF_GameLog_Callback_Handle SPF_GameLog_Callback_Handle;

/**
 * @brief Function signature for the game log monitor callback.
 * @param message The raw text of the log line.
 * @param userData User-defined pointer passed during registration.
 */
typedef void (*SPF_GameLog_Callback_t)(const char* message, void* userData);

// =================================================================================================
// GameLog API Structure
// =================================================================================================

/**
 * @struct SPF_GameLog_API
 * @brief Table of function pointers for log monitoring operations.
 */
typedef struct SPF_GameLog_API {

    /**
     * @brief Retrieves the log monitoring context for a specific plugin.
     * @param pluginName Unique ID of the plugin.
     * @return A handle to the GameLog context, or NULL if initialization fails.
     */
    SPF_GameLog_Handle* (*GLog_GetContext)(const char* pluginName);

    /**
     * @brief Registers a callback to receive real-time game log updates.
     * @details Every line written to the game's log will be passed to the provided function.
     * 
     * @param h The context handle obtained from GLog_GetContext.
     * @param callback The function to invoke for each log line.
     * @param userData Optional pointer passed back to the callback.
     * @return A handle to the subscription, or NULL on failure.
     */
    SPF_GameLog_Callback_Handle* (*GLog_RegisterCallback)(SPF_GameLog_Handle* h, SPF_GameLog_Callback_t callback, void* userData);

} SPF_GameLog_API;

#ifdef __cplusplus
}
#endif