#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {
// Forward declare the service class
class GameObjectFileSystemService;

/**
 * @class IFileSystemDataFinder
 * @brief An interface for classes that find UFS (FileSystem) related offsets or pointers.
 */
class IFileSystemDataFinder {
 public:
  virtual ~IFileSystemDataFinder() = default;

  virtual const char* GetName() const = 0;

  /**
   * @brief Attempts to find memory offsets and pointers for UFS.
   * @param owner The GameObjectFileSystemService to populate.
   * @return True if successful.
   */
  virtual bool TryFindOffsets(GameObjectFileSystemService& owner) = 0;

  /**
   * @brief Checks if the finder has successfully found all its required data.
   * @return True if ready, false otherwise.
   */
  bool IsReady() const { return m_isReady; }

 protected:
  bool m_isReady = false;
};

}  // namespace Data::GameData
SPF_NS_END
