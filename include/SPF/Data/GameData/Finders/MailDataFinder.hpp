/**
 * @file MailDataFinder.hpp
 * @brief Dynamic pattern searcher for game mail system functions.
 */

#pragma once

namespace SPF::Data::GameData {
class MailService;
}

namespace SPF::Data::GameData::Finders {

/**
 * @class MailDataFinder
 * @brief Resolves mail system function addresses at runtime and writes
 *        results directly into the owning MailService (Session pattern).
 */
class MailDataFinder {
 public:
  /**
   * @brief Scans for all mail system patterns and writes offsets/addrs to @p owner.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFind(MailService& owner);

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const { return "MailDataFinder"; }

 private:
  bool m_isReady = false;
};

}  // namespace SPF::Data::GameData::Finders
