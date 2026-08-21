/**
 * @file IClimateDataFinder.hpp
 * @brief Interface for Climate data finders.
 */

#pragma once
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {

// Forward declaration
class ClimateService;

/**
 * @interface IClimateDataFinder
 * @brief Defines the interface for all Climate data finders.
 */
class IClimateDataFinder {
 public:
  virtual ~IClimateDataFinder() = default;

  /**
   * @brief Attempts to find climate data offsets and addresses.
   * @param owner Reference to the ClimateService to store found data.
   * @return true if all critical data was found.
   */
  virtual bool TryFindOffsets(ClimateService& owner) = 0;

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
  void Reset() { m_isReady = false; }

 protected:
  bool m_isReady = false;
};

}  // namespace Data::GameData
SPF_NS_END
