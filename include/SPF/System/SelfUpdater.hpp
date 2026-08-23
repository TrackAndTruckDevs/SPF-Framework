#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/System/ApiService.hpp"

#include <filesystem>
#include <future>
#include <optional>
#include <string>

SPF_NS_BEGIN
namespace System {

/**
 * @brief Result of a patch apply attempt.
 */
struct PatchApplyResult {
  bool success = false;
  std::string version;
  std::string errorMessage;
};

/**
 * @brief Downloads a framework hotfix patch, verifies its MD5 and swaps the running DLL file.
 * @details The old DLL is renamed to ".old" (allowed while loaded) and removed on the next
 *          startup via StartupCleanup(). Only one apply attempt can be in flight at a time;
 *          results are consumed by polling PollResult() from the main loop.
 */
class SelfUpdater {
 public:
  explicit SelfUpdater(ApiService& apiService);

  /**
   * @brief Starts an asynchronous patch apply for the given update info. Ignored if already busy.
   */
  void ApplyPatchAsync(const UpdateInfo& info);

  /**
   * @brief Non-blocking poll. Returns and clears the result when the apply flow has finished.
   */
  std::optional<PatchApplyResult> PollResult();

  /**
   * @brief Removes leftovers from previous sessions (.old backup next to the DLL and temp files).
   */
  static void StartupCleanup();

 private:
  PatchApplyResult Finalize(const FileDownloadResult& download);
  void Finish(const PatchApplyResult& result);

  ApiService& m_apiService;
  std::optional<std::future<FileDownloadResult>> m_downloadFuture;
  std::optional<std::future<PatchApplyResult>> m_finalizeFuture;
  std::optional<PatchApplyResult> m_lastResult;
  std::string m_targetVersion;
  std::filesystem::path m_tempFilePath;
  std::string m_expectedMd5;
  bool m_busy = false;
};

}  // namespace System
SPF_NS_END
