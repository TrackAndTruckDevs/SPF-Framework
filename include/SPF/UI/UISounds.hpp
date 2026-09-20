#pragma once

#include <cstdint>

namespace SPF::UI {

class UISounds {
 public:
  static void OnMouseClicked();
  static void PlayClickSound();
  static void PlayMessageSound();
  static void ResetCachedEvents();

  static void SetEnabled(bool enabled);
  static bool IsEnabled();

 private:
  static constexpr char kClickEventPath[] = "event:/click";
  static constexpr char kMessageEventPath[] = "event:/message";
  static bool s_clickGuidCached;
  static uint8_t s_clickGuid[16];
  static bool s_clickGuidValid;
  static bool s_messageGuidCached;
  static uint8_t s_messageGuid[16];
  static bool s_messageGuidValid;
  static bool s_enabled;

  static bool EnsureClickGuidCached();
  static bool EnsureMessageGuidCached();
};

}  // namespace SPF::UI
