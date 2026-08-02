/**
 * @file IManagerDataFinder.hpp
 * @brief Interface for core manager data finders.
 */

#pragma once
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {

// Forward declaration
class ManagerCoreService;

/**
 * @interface IManagerDataFinder
 * @brief Defines the interface for all core manager data finders.
 */
class IManagerDataFinder {
 public:
  virtual ~IManagerDataFinder() = default;

  /**
   * @brief Attempts to find manager addresses and pointers.
   * @param owner Reference to the ManagerCoreService to store found data.
   * @return true if all critical data was found.
   */
  virtual bool TryFindOffsets(ManagerCoreService& owner) = 0;

  /**
   * @brief Returns the name of the finder for logging purposes.
   * @return A C-style string name of the finder.
   */
  virtual const char* GetName() const = 0;

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
