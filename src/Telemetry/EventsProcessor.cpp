#include "SPF/Telemetry/EventsProcessor.hpp"

#include <cstring>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Telemetry/GameContext.hpp"

// SDK headers for event and attribute names
#include "common/scssdk_telemetry_common_gameplay_events.h"

SPF_NS_BEGIN
namespace Telemetry {

EventsProcessor::EventsProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context), m_lastGameplayEventId("") {}

void EventsProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("EventsProcessor initialized."); }

void EventsProcessor::Shutdown() { m_logger.Info("EventsProcessor shut down."); }

const std::string& EventsProcessor::GetLastGameplayEventId() const { return m_lastGameplayEventId; }

void EventsProcessor::HandleFrameStart() {
  // Reset all single-frame event flags.
  memset(&m_specialEvents, 0, sizeof(m_specialEvents));
}

void EventsProcessor::HandleGameplayEvent(const scs_telemetry_gameplay_event_t* info) {
  if (!info || !info->id) return;

  m_lastGameplayEventId = info->id;
  ConfigAttributeReader reader(info->attributes);

  if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_job_delivered) == 0) {
    m_logger.Info("[Event] Job Delivered");
    m_specialEvents.job_delivered = true;

    auto& data = m_gameplayEvents.job_delivered;
    data.revenue = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_revenue).value_or(0);
    data.earned_xp = reader.GetS32(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_earned_xp).value_or(0);
    data.cargo_damage = reader.GetFloat(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_cargo_damage).value_or(0.0f);
    data.distance_km = reader.GetFloat(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_distance_km).value_or(0.0f);
    data.delivery_time = reader.GetU32(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_delivery_time).value_or(0);
    data.auto_park_used = reader.GetBool(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_auto_park_used).value_or(false);
    data.auto_load_used = reader.GetBool(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_auto_load_used).value_or(false);
  } else if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_job_cancelled) == 0) {
    m_logger.Info("[Event] Job Cancelled");
    m_specialEvents.job_cancelled = true;

    auto& data = m_gameplayEvents.job_cancelled;
    data.penalty = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_cancel_penalty).value_or(0);
  } else if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_player_fined) == 0) {
    m_logger.Info("[Event] Player Fined");
    m_specialEvents.fined = true;

    auto& data = m_gameplayEvents.player_fined;
    data.fine_amount = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_fine_amount).value_or(0);
    data.fine_offence = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_fine_offence).value_or("");
  } else if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_player_tollgate_paid) == 0) {
    m_logger.Info("[Event] Tollgate Paid");
    m_specialEvents.tollgate = true;

    auto& data = m_gameplayEvents.tollgate_paid;
    data.pay_amount = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_pay_amount).value_or(0);
  } else if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_ferry) == 0) {
    m_logger.Info("[Event] Ferry Used");
    m_specialEvents.ferry = true;

    auto& data = m_gameplayEvents.ferry_used;
    data.pay_amount = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_pay_amount).value_or(0);
    data.source_name = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_source_name).value_or("");
    data.target_name = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_target_name).value_or("");
    data.source_id = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_source_id).value_or("");
    data.target_id = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_target_id).value_or("");
  } else if (strcmp(info->id, SCS_TELEMETRY_GAMEPLAY_EVENT_player_use_train) == 0) {
    m_logger.Info("[Event] Train Used");
    m_specialEvents.train = true;

    auto& data = m_gameplayEvents.train_used;
    data.pay_amount = reader.GetS64(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_pay_amount).value_or(0);
    data.source_name = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_source_name).value_or("");
    data.target_name = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_target_name).value_or("");
    data.source_id = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_source_id).value_or("");
    data.target_id = reader.GetString(SCS_TELEMETRY_GAMEPLAY_EVENT_ATTRIBUTE_target_id).value_or("");
  }

}

}  // namespace Telemetry
SPF_NS_END
