#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Data/GameData/IFileSystemDataFinder.hpp"
#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"
#include <string>

SPF_NS_BEGIN
namespace Data::GameData {

GameObjectFileSystemService& GameObjectFileSystemService::GetInstance() {
    static GameObjectFileSystemService instance;
    return instance;
}

void GameObjectFileSystemService::Initialize() {
    if (m_isInitialized) return;
    RegisterFinders();
    m_isInitialized = true;
}

void GameObjectFileSystemService::RegisterFinders() {
    m_dataFinders.push_back(std::make_unique<Finders::FileSystemDataFinder>());
}

bool GameObjectFileSystemService::TryFindAllOffsets() {
    bool allFound = true;
    for (auto& finder : m_dataFinders) {
        if (!finder->IsReady()) {
            if (!finder->TryFindOffsets(*this)) {
                allFound = false;
            }
        }
    }
    return allFound;
}

void GameObjectFileSystemService::Reset() {
    m_homePathPtrAddr = 0;
    m_devicesArrayAddr = 0;
    m_managersCountAddr = 0;
    m_gamePtrAddr = 0;
    m_gamePtrAdjustment = 0;
    m_profileHandleOffset = 0;
    m_mountListHeadOffset = 0;
    m_physDevicePathOffset = 0;
    m_nodeDeviceOffset = 0;
    m_nodeVPathOffset = 0;
    m_stringBufferOffset = 0;
    
    for (auto& finder : m_dataFinders) {
        // In the current architecture, finders handle their own state
    }
}

bool GameObjectFileSystemService::AreAllFindersReady() const {
    if (m_dataFinders.empty()) return false;
    for (const auto& finder : m_dataFinders) {
        if (!finder->IsReady()) return false;
    }
    return true;
}

bool GameObjectFileSystemService::IsFinderReady(const char* finderName) const {
    for (const auto& finder : m_dataFinders) {
        if (std::string(finder->GetName()) == finderName) {
            return finder->IsReady();
        }
    }
    return false;
}

} // namespace Data::GameData
SPF_NS_END
