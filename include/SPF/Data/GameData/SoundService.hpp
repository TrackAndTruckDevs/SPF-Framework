#pragma once

#include "SPF/Data/GameData/Finders/ISoundDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SPF::Data::GameData {

struct SoundEvent {
  std::string bankPath;
  std::string eventPath;
  uint8_t guid[16];
};

struct SoundBankGroup {
  std::string bankPath;
  std::vector<SoundEvent> events;
};

class SoundService : public IWorldScopedService {
 public:
  static SoundService& GetInstance();

  SoundService(const SoundService&) = delete;
  void operator=(const SoundService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady();
  bool TryFindAllOffsets();

  // Returns sounds grouped by bank.
  std::vector<SoundBankGroup> GetSoundBankGroups();

  // --- IWorldScopedService ---
  const char* GetName() const override { return "SoundService"; }
  void ResetForWorldReload() override { Shutdown(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }

  // --- Getters ---
  uint32_t GetBankListLockOffset() const { return m_bankListLockOffset; }
  uint32_t GetBankListHeadOffset() const { return m_bankListHeadOffset; }
  uint32_t GetBankListSentinelOffset() const { return m_bankListSentinelOffset; }
  uint32_t GetBankEventListHeadOffset() const { return m_bankEventListHeadOffset; }
  uint32_t GetBankPathStringOffset() const { return m_bankPathStringOffset; }
  uint32_t GetEventListTerminatorOffset() const { return m_eventListTerminatorOffset; }
  uint32_t GetEventPathOffset() const { return m_eventPathOffset; }
  uint32_t GetEventGuidOffset() const { return m_eventGuidOffset; }

  // --- Setters (for finders) ---
  void SetBankListLockOffset(uint32_t off) { m_bankListLockOffset = off; }
  void SetBankListHeadOffset(uint32_t off) { m_bankListHeadOffset = off; }
  void SetBankListSentinelOffset(uint32_t off) { m_bankListSentinelOffset = off; }
  void SetBankEventListHeadOffset(uint32_t off) { m_bankEventListHeadOffset = off; }
  void SetBankPathStringOffset(uint32_t off) { m_bankPathStringOffset = off; }
  void SetEventListTerminatorOffset(uint32_t off) { m_eventListTerminatorOffset = off; }
  void SetEventPathOffset(uint32_t off) { m_eventPathOffset = off; }
  void SetEventGuidOffset(uint32_t off) { m_eventGuidOffset = off; }

 private:
  SoundService();
  ~SoundService() = default;

  void RegisterFinders();

  bool m_isInitialized = false;
  std::vector<std::unique_ptr<ISoundDataFinder>> m_dataFinders;

  uint32_t m_bankListLockOffset = 0;
  uint32_t m_bankListHeadOffset = 0;
  uint32_t m_bankListSentinelOffset = 0;
  uint32_t m_bankEventListHeadOffset = 0;
  uint32_t m_bankPathStringOffset = 0;
  uint32_t m_eventListTerminatorOffset = 0;
  uint32_t m_eventPathOffset = 0;
  uint32_t m_eventGuidOffset = 0;
};

}  // namespace SPF::Data::GameData
