#include "SPF/UI/UISounds.hpp"

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include "imgui.h"
#include <cstdint>

namespace SPF::UI {

bool UISounds::s_clickGuidCached = false;
uint8_t UISounds::s_clickGuid[16] = {};
bool UISounds::s_clickGuidValid = false;
bool UISounds::s_enabled = true;

void UISounds::ResetCachedEvents() {
  s_clickGuidCached = false;
  s_clickGuidValid = false;
}

bool UISounds::EnsureClickGuidCached() {
  if (s_clickGuidCached) return s_clickGuidValid;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UISounds");
  auto& sound = Data::GameData::SoundService::GetInstance();

  logger->Info("EnsureClickGuidCached: SoundService.IsReady={}", sound.IsReady());

  if (!sound.IsReady()) {
    s_clickGuidCached = true;
    s_clickGuidValid = false;
    return false;
  }

  s_clickGuidValid = sound.FindEventGuidByPath(kClickEventPath, s_clickGuid);
  s_clickGuidCached = true;

  if (!s_clickGuidValid) {
    logger->Warn("EnsureClickGuidCached: '{}' not found in loaded banks", kClickEventPath);
  }
  return s_clickGuidValid;
}

void UISounds::OnMouseClicked() {
  if (!s_enabled) return;
  auto& io = ImGui::GetIO();
  if (!io.MouseClicked[0]) return;
  if (!io.WantCaptureMouse) return;
  if (!ImGui::IsAnyItemHovered()) return;
  PlayClickSound();
}

void UISounds::PlayClickSound() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UISounds");

  if (!EnsureClickGuidCached()) {
    static int skipCount = 0;
    if (++skipCount <= 3) {
      logger->Warn("PlayClickSound: skipping (event not ready/found), skip #{}", skipCount);
    }
    return;
  }

  auto& sound = Data::GameData::SoundService::GetInstance();
  if (!sound.IsReady()) {
    static int notReadyCount = 0;
    if (++notReadyCount <= 3) {
      logger->Warn("PlayClickSound: SoundService not ready, count #{}", notReadyCount);
    }
    s_clickGuidCached = false;
    return;
  }

  auto* instance = sound.CreateEventInstance(s_clickGuid);
  if (!instance) {
    static int createFailCount = 0;
    if (++createFailCount <= 3) {
      logger->Warn("PlayClickSound: CreateEventInstance returned null, count #{}", createFailCount);
    }
    s_clickGuidCached = false;
    return;
  }

  bool started = sound.StartEvent(instance);
  if (!started) {
    logger->Warn("PlayClickSound: StartEvent failed");
  }

  sound.ReleaseEventInstance(instance);
}

void UISounds::SetEnabled(bool enabled) { s_enabled = enabled; }
bool UISounds::IsEnabled() { return s_enabled; }

}  // namespace SPF::UI
