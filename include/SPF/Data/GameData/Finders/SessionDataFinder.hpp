#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ISessionDataFinder.hpp"


SPF_NS_BEGIN
namespace Data::GameData::Finders {

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

}  // namespace Data::GameData::Finders
SPF_NS_END
