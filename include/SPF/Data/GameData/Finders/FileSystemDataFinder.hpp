#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IFileSystemDataFinder.hpp"


SPF_NS_BEGIN
namespace Data::GameData::Finders {

/**
 * @class FileSystemDataFinder
 * @brief Finds pointers and offsets related to the Prism FileSystem (UFS).
 */
class FileSystemDataFinder : public IFileSystemDataFinder {
 public:
  const char* GetName() const override { return "FileSystemDataFinder"; }

 protected:
  bool TryFindOffsets(GameObjectFileSystemService& owner) override;
};

}  // namespace Data::GameData::Finders
SPF_NS_END
