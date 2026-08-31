#pragma once

#include "SPF/Data/GameData/ISessionDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

class SessionDataFinder : public ISessionDataFinder {
 public:
  SessionDataFinder() = default;
  virtual ~SessionDataFinder() = default;

  bool TryFindOffsets(GameObjectSessionService& owner) override;
  bool IsReady() const override { return m_isReady; }
  const char* GetName() const override { return "SessionDataFinder"; }

 private:
  bool m_isReady = false;
};

}  // namespace SPF::Data::GameData::Finders
