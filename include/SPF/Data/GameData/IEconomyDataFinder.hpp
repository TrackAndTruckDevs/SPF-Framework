/**
 * @file IEconomyDataFinder.hpp
 * @brief Interface for economy data finders.
 */

#pragma once

namespace SPF::Data::GameData {

// Forward declaration
class EconomyService;

/**
 * @interface IEconomyDataFinder
 * @brief Defines the interface for all economy data finders.
 */
class IEconomyDataFinder {
 public:
  virtual ~IEconomyDataFinder() = default;

  /**
   * @brief Attempts to find economy addresses and offsets.
   * @param owner Reference to the EconomyService to store found data.
   * @return true if all critical data was found.
   */
  virtual bool TryFindOffsets(EconomyService& owner) = 0;

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

}  // namespace SPF::Data::GameData
