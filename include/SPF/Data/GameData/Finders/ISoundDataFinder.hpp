#pragma once

namespace SPF::Data::GameData {

class SoundService;

class ISoundDataFinder {
 public:
  virtual ~ISoundDataFinder() = default;
  virtual bool TryFindOffsets(SoundService& owner) = 0;
  virtual const char* GetName() const = 0;

  bool IsReady() const { return m_isReady; }
  void Reset() { m_isReady = false; }

 protected:
  bool m_isReady = false;
};

}  // namespace SPF::Data::GameData
