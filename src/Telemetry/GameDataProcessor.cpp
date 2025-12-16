#include "SPF/Telemetry/GameDataProcessor.hpp"

#include <cstring>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Events/EventManager.hpp"

SPF_NS_BEGIN
namespace Telemetry {
using namespace SPF::Logging;

GameDataProcessor::GameDataProcessor(Logger& logger, GameContext& context, Events::EventManager& eventManager)
    : m_logger(logger), m_context(context), m_eventManager(eventManager) {}

void GameDataProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) {
  m_gameState.game_id = m_context.GetGame();
  m_gameState.game_name = scs_params->common.game_name;

  // SCS Game Version
  m_gameState.scs_game_version_major = SCS_GET_MAJOR_VERSION(scs_params->common.game_version);
  m_gameState.scs_game_version_minor = SCS_GET_MINOR_VERSION(scs_params->common.game_version);

  // Telemetry Plugin SDK Version
  m_gameState.telemetry_plugin_version_major = SCS_GET_MAJOR_VERSION(SCS_TELEMETRY_VERSION_CURRENT);
  m_gameState.telemetry_plugin_version_minor = SCS_GET_MINOR_VERSION(SCS_TELEMETRY_VERSION_CURRENT);
  
  // Game-specific Telemetry SDK Version
  if (m_context.IsETS2()) {
    m_gameState.telemetry_game_version_major = SCS_GET_MAJOR_VERSION(SCS_TELEMETRY_EUT2_GAME_VERSION_CURRENT);
    m_gameState.telemetry_game_version_minor = SCS_GET_MINOR_VERSION(SCS_TELEMETRY_EUT2_GAME_VERSION_CURRENT);
  } else if (m_context.IsATS()) {
    m_gameState.telemetry_game_version_major = SCS_GET_MAJOR_VERSION(SCS_TELEMETRY_ATS_GAME_VERSION_CURRENT);
    m_gameState.telemetry_game_version_minor = SCS_GET_MINOR_VERSION(SCS_TELEMETRY_ATS_GAME_VERSION_CURRENT);
  }

  m_logger.Info("GameDataProcessor initialized for %s.", m_gameState.game_name.c_str());
  m_logger.Info(
    "Versions: Game %u.%u, Telemetry SDK %u.%u, Game-specific SDK %u.%u",
    m_gameState.scs_game_version_major,
    m_gameState.scs_game_version_minor,
    m_gameState.telemetry_plugin_version_major,
    m_gameState.telemetry_plugin_version_minor,
    m_gameState.telemetry_game_version_major,
    m_gameState.telemetry_game_version_minor
  );
}

void GameDataProcessor::Shutdown() { m_gameWorldReadyNotified = false; }

void GameDataProcessor::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  if (strcmp(info->id, SCS_TELEMETRY_CONFIG_substances) == 0) {
    m_logger.Info("GameDataProcessor handling substances configuration...");
    m_commonData.substances.clear();
    ConfigAttributeReader reader(info->attributes);
    for (uint32_t i = 0;; ++i) {
      auto substance_name = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_id, i);
      if (substance_name) {
        m_commonData.substances.push_back(*substance_name);
      } else {
        break;  // No more substances
      }
    }
    m_logger.Info("Registered %zu substances.", m_commonData.substances.size());
  }
}

void GameDataProcessor::HandleChannelUpdate(const scs_string_t name, const scs_u32_t, const scs_value_t* value) {
  if (!value || !name) return;

  if (strcmp(name, SCS_TELEMETRY_CHANNEL_game_time) == 0) {
    m_commonData.game_time = value->value_u32.value;
    RecalculateRestStopTime();
  } else if (strcmp(name, SCS_TELEMETRY_CHANNEL_local_scale) == 0) {
    m_gameState.scale = value->value_float.value;
    RecalculateRealTimeDurations();
  } else if (strcmp(name, SCS_TELEMETRY_CHANNEL_next_rest_stop) == 0) {
    m_commonData.next_rest_stop = value->value_s32.value;
    RecalculateRestStopTime();
    RecalculateRealTimeDurations();
  } else if (strcmp(name, SCS_TELEMETRY_CHANNEL_multiplayer_time_offset) == 0) {
    m_gameState.multiplayer_time_offset = value->value_s32.value;
  }
}

void GameDataProcessor::HandleFrameStart(const scs_telemetry_frame_start_t* const info) {
  if (!info) return;
  m_timestamps.simulation = info->simulation_time;
  m_timestamps.render = info->render_time;
  m_timestamps.paused_simulation = info->paused_simulation_time;

  if (m_timestamps.simulation > 0 && !m_gameWorldReadyNotified) {
    m_eventManager.System.OnGameWorldReady.Call();
    m_gameWorldReadyNotified = true;
    m_logger.Info("Game world is now considered ready (simulation > 0).");
  }
}

void GameDataProcessor::HandlePaused() { m_gameState.paused = true; }

void GameDataProcessor::HandleStarted() { m_gameState.paused = false; }

void GameDataProcessor::RecalculateRestStopTime() {
  if (m_commonData.next_rest_stop < 0) {
    // No rest stop scheduled
    return;
  }

  const uint32_t total_minutes = m_commonData.game_time + m_commonData.next_rest_stop;

  const uint32_t minutes_in_day = 24 * 60;
  const uint32_t minutes_in_week = minutes_in_day * 7;

  const uint32_t week_minutes = total_minutes % minutes_in_week;

  const uint32_t day_minutes = week_minutes % minutes_in_day;

  // Assuming game starts on Monday (day 1)
  m_commonData.next_rest_stop_time.DayOfWeek = (week_minutes / minutes_in_day) + 1;
  m_commonData.next_rest_stop_time.Hour = day_minutes / 60;
  m_commonData.next_rest_stop_time.Minute = day_minutes % 60;
}

void GameDataProcessor::RecalculateRealTimeDurations() {
  if (m_gameState.scale > 0.0f && m_commonData.next_rest_stop >= 0) {
    m_commonData.next_rest_stop_real_minutes = m_commonData.next_rest_stop / m_gameState.scale;
  } else {
    m_commonData.next_rest_stop_real_minutes = 0.0f;
  }
}

}  // namespace Telemetry
SPF_NS_END
