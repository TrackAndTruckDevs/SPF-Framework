#pragma once

#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Hooks/IHook.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace SPF::Fmod {

struct Override3DData {
  FMOD_3D_ATTRIBUTES original{};
  FMOD_3D_ATTRIBUTES replacement{};
  bool hasOriginal = false;
};

struct OverrideListenerData {
  FMOD_3D_ATTRIBUTES original{};
  FMOD_3D_ATTRIBUTES replacement{};
  bool hasOriginal = false;
};

class FmodStudioHook : public Hooks::IHook {
 public:
  static FmodStudioHook& GetInstance();

  FmodStudioHook(const FmodStudioHook&) = delete;
  FmodStudioHook& operator=(const FmodStudioHook&) = delete;

  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override;
  const std::string& GetSignature() const override { return m_signature; }
  bool IsInstalled() const override { return m_installed; }
  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  void OverrideParameter(const std::string& eventPath, const std::string& paramName, float value);
  void Override3DAttributes(const std::string& eventPath, const FMOD_3D_ATTRIBUTES& attrs);
  void Override3DPosition(const std::string& eventPath, const FMOD_VECTOR& position);
  void Reset3DToOriginal(const std::string& eventPath);
  void RemoveParameterOverride(const std::string& eventPath, const std::string& paramName);
  void Remove3DOverride(const std::string& eventPath);

  void OverrideListenerAttributes(int index, const FMOD_3D_ATTRIBUTES& attrs);
  void RemoveListenerOverride(int index);
  void ResetListenerToOriginal(int index);
  void RemoveAllOverrides();
  bool HasOverrides() const;

  // Activity codes delivered to ActivityCallback — keep in sync with
  // SPF_ACTIVITY_* in SPF_Sound_API.h.
  static constexpr int kActivityEventCreated = 1;
  static constexpr int kActivityStartSuppressed = 2;
  static constexpr int kActivityStarted = 3;
  static constexpr int kActivityStopped = 4;
  static constexpr int kActivityPaused = 5;
  static constexpr int kActivityUnpaused = 6;
  static constexpr int kActivityReleased = 7;
  static constexpr int kActivityParamSet = 8;
  static constexpr int kActivityBankLoaded = 9;
  static constexpr int kActivityBankUnloading = 10;

  // Invoked synchronously from inside the intercepting detour, on the thread
  // that made the FMOD call (normally the game thread). Path/param buffers are
  // valid only for the duration of the call.
  using ActivityCallback = void (*)(void* user_data, int activity, const char* path,
                                    void* instance, const char* param_name, float param_value);

  // Blocks EventInstance::start for every event path beginning with the prefix:
  // the detour returns FMOD_OK without playing. Returns false when the start
  // hook is unavailable (suppression would be a silent no-op).
  bool SuppressEventPlayback(const std::string& pathPrefix);
  bool UnsuppressEventPlayback(const std::string& pathPrefix);
  uint64_t GetSuppressedStartCount(const std::string& pathPrefix) const;
  void* GetLastSuppressedInstance(const std::string& pathPrefix) const;
  void SetActivityCallback(ActivityCallback callback, void* userData);
  bool HasActivityCallback() const;

  void OverrideEventVolume(const std::string& eventPath, float volume);
  void RemoveEventVolumeOverride(const std::string& eventPath);
  void OverrideEventPitch(const std::string& eventPath, float pitch);
  void RemoveEventPitchOverride(const std::string& eventPath);

  void PopulateDescPathCache(void* desc, const char* path);
  void ClearDescPathCache();

  // Optional fallback resolver for event paths that FMOD cannot resolve
  // internally (rc=74 for SCS memory-loaded banks). Invoked from inside
  // GetEventPathFromInstance when getPath/lookupPath both fail but the event
  // GUID is available (idRc==0). The callback receives the 16-byte event GUID
  // and the EventInstance pointer, and returns a NUL-terminated path (or
  // nullptr when unresolved). SoundService registers a resolver backed by the
  // game's per-bank data, which knows event paths FMOD's path table does not.
  using PathResolverCallback = const char* (*)(void* user_data, const uint8_t guid[16],
                                                void* instance);
  void SetPathResolverCallback(PathResolverCallback callback, void* userData);
  bool HasPathResolverCallback() const;

 private:
  FmodStudioHook();

  std::string m_name = "FmodStudioHook";
  std::string m_displayName = "FMOD Studio";
  std::string m_ownerName = "framework";
  std::string m_signature;
  bool m_isEnabled = true;
  bool m_installed = false;

  uintptr_t m_hookedAddrSetParamByName = 0;
  uintptr_t m_hookedAddrSetParamByID = 0;
  uintptr_t m_hookedAddrSet3DAttributes = 0;
  uintptr_t m_hookedAddrSetListenerAttributes = 0;
  // Optional interception hooks (start/stop/create/release/paused/volume/pitch/
  // banks) — recorded only for exports that were found and installed.
  std::vector<uintptr_t> m_extraHookedAddrs;
};

}  // namespace SPF::Fmod
