#pragma once

#include "SPF/Hooks/IHook.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

using FMOD_RESULT = int32_t;
constexpr FMOD_RESULT FMOD_OK = 0;

namespace FMOD {
class System;
class ChannelGroup;
namespace Studio {
class System;
class Bank;
class Bus;
class VCA;
class EventDescription;
class EventInstance;
}  // namespace Studio
}  // namespace FMOD

struct FMOD_GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
};

struct FMOD_VECTOR {
  float x, y, z;
};

struct FMOD_3D_ATTRIBUTES {
  FMOD_VECTOR position;
  FMOD_VECTOR velocity;
  FMOD_VECTOR forward;
  FMOD_VECTOR up;
};

struct FMOD_STUDIO_PARAMETER_ID {
  uint32_t data1;
  uint32_t data2;
};

enum class FMOD_STUDIO_PARAMETER_TYPE : int32_t {
  GAME_CONTROLLED = 0,
  AUTOMATIC_DISTANCE = 1,
  AUTOMATIC_EVENT_CONE_ANGLE = 2,
  AUTOMATIC_EVENT_ORIENTATION = 3,
  AUTOMATIC_DIRECTION = 4,
  AUTOMATIC_ELEVATION = 5,
  AUTOMATIC_LISTENER_ORIENTATION = 6,
  AUTOMATIC_SPEED = 7,
  AUTOMATIC_SPEED_ABSOLUTE = 8,
  AUTOMATIC_DISTANCE_NORMALIZED = 9,
  MAX = 10,
};

enum class FMOD_STUDIO_PARAMETER_FLAGS : uint32_t {
  NONE = 0,
  AUTOMATIC_DISTANCE = 1,
  AUTOMATIC_EVENT_CONE_ANGLE = 2,
  AUTOMATIC_EVENT_ORIENTATION = 4,
  AUTOMATIC_DIRECTION = 8,
  AUTOMATIC_ELEVATION = 16,
  AUTOMATIC_LISTENER_ORIENTATION = 32,
  AUTOMATIC_SPEED = 64,
  AUTOMATIC_SPEED_ABSOLUTE = 128,
  AUTOMATIC_DISTANCE_NORMALIZED = 256,
};

using FMOD_BOOL = int;

typedef int FMOD_STUDIO_USER_PROPERTY_TYPE;
constexpr FMOD_STUDIO_USER_PROPERTY_TYPE FMOD_STUDIO_USER_PROPERTY_TYPE_BOOLEAN = 0;
constexpr FMOD_STUDIO_USER_PROPERTY_TYPE FMOD_STUDIO_USER_PROPERTY_TYPE_INTEGER = 1;
constexpr FMOD_STUDIO_USER_PROPERTY_TYPE FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT = 2;
constexpr FMOD_STUDIO_USER_PROPERTY_TYPE FMOD_STUDIO_USER_PROPERTY_TYPE_STRING = 3;

struct FMOD_STUDIO_USER_PROPERTY {
  const char* name;
  FMOD_STUDIO_USER_PROPERTY_TYPE type;
  union {
    int intvalue;
    float floatvalue;
    FMOD_BOOL boolvalue;
    const char* stringvalue;
  };
};

struct FMOD_STUDIO_PARAMETER_DESCRIPTION {
  const char* name;
  FMOD_STUDIO_PARAMETER_ID id;
  float minimum;
  float maximum;
  float defaultvalue;
  FMOD_STUDIO_PARAMETER_TYPE type;
  FMOD_STUDIO_PARAMETER_FLAGS flags;
  FMOD_GUID guid;
};

using FMOD_MODE = uint32_t;
constexpr FMOD_MODE FMOD_LOOP_OFF = 0x00000001;
constexpr FMOD_MODE FMOD_LOOP_NORMAL = 0x00000002;
constexpr FMOD_MODE FMOD_LOOP_BIDI = 0x00000004;

enum class FMOD_STUDIO_STOP_MODE : int32_t { ALLOWFADEOUT = 0, IMMEDIATE = 1 };
enum class FMOD_STUDIO_PLAYBACK_STATE : int32_t { PLAYING = 0, SUSTAINING, STOPPED, STARTING, STOPPING };
enum class FMOD_STUDIO_EVENT_PROPERTY : int32_t { PRIORITY = 0, CHANNELPRIORITY, SCHEDULE_DELAY, SCHEDULE_LOOKAHEAD, MINIMUM_DISTANCE, MAXIMUM_DISTANCE, COUNT };

namespace SPF::Fmod {

class FmodApi : public Hooks::IHook {
 public:
  static FmodApi& GetInstance();

  FmodApi() = default;
  FmodApi(const FmodApi&) = delete;
  void operator=(const FmodApi&) = delete;

  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  const std::string& GetSignature() const override { return m_signature; }
  bool IsInstalled() const override { return m_ready; }
  bool Install() override;
  void Uninstall() override;
  void Remove() override { Uninstall(); }

  void* Find(const char* demangledName) const;
  bool HasFunction(const char* demangledName) const;
  bool IsReady() const { return m_ready; }
  const std::unordered_map<std::string, void*>& GetExports() const { return m_exports; }
  size_t GetExportCount() const { return m_exports.size(); }

 private:
  bool m_isEnabled = true;
  bool m_ready = false;
  std::string m_name = "FmodApi";
  std::string m_displayName = "FMOD API";
  std::string m_ownerName = "framework";
  std::string m_signature;
  std::unordered_map<std::string, void*> m_exports;

  static void ParseExportTable(uintptr_t base, const char* dllName, std::unordered_map<std::string, void*>& out);
  static std::string StripFMODMangling(const char* mangled);
};

}  // namespace SPF::Fmod
