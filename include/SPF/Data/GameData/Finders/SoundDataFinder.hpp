#pragma once

#include "SPF/Data/GameData/Finders/ISoundDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

class SoundDataFinder : public ISoundDataFinder {
 public:
  bool TryFindOffsets(SoundService& owner) override;
  const char* GetName() const override { return "SoundDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
