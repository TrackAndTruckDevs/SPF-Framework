#include "SPF/Data/GameData/Finders/CabinCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool CabinCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Cabin Camera offsets...");

  // No offsets to find for this camera yet
  m_isReady = true;
  logger->Info("Successfully found all cabin camera data (none to find).");
  return true;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
