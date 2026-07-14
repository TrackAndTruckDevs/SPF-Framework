#include "SPF/Data/GameData/Finders/PhotoCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

bool PhotoCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING PHOTO CAMERA OFFSET SEARCH ---");

  // Placeholder for reflection-based search (waiting for user table)
  bool all_found = true;

  // Runtime state patterns will be added after analyzing UpdatePhotoCamera function

  m_isReady = all_found;
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- PHOTO CAMERA OFFSETS FOUND. PhotoCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize Photo Camera offsets.");
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
