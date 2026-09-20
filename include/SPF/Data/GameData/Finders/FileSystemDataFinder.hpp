#pragma once

#include "SPF/Data/GameData/IFileSystemDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

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

}  // namespace SPF::Data::GameData::Finders
