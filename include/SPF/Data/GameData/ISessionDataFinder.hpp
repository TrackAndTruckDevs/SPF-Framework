#pragma once

namespace SPF::Data::GameData {

class GameObjectSessionService;

/**
 * @class ISessionDataFinder
 * @brief Base interface for classes that search for game session and profile related memory addresses.
 */
class ISessionDataFinder {
 public:
  virtual ~ISessionDataFinder() = default;
  virtual bool TryFindOffsets(GameObjectSessionService& owner) = 0;
  virtual bool IsReady() const = 0;
  virtual const char* GetName() const = 0;
};

}  // namespace SPF::Data::GameData
