#pragma once

#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Hooks/IHook.hpp"

#include <cstdint>
#include <string>

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

  void PopulateDescPathCache(void* desc, const char* path);
  void ClearDescPathCache();

 private:
  FmodStudioHook();

  std::string m_name = "FmodStudioHook";
  std::string m_displayName = "FMOD Studio";
  std::string m_ownerName = "framework";
  std::string m_signature;
  bool m_isEnabled = true;
  bool m_installed = false;

  uintptr_t m_hookedAddr1 = 0;
  uintptr_t m_hookedAddr2 = 0;
  uintptr_t m_hookedAddr3 = 0;
  uintptr_t m_hookedAddr4 = 0;
};

}  // namespace SPF::Fmod
