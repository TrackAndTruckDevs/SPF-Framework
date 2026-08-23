#include "SPF/System/SelfUpdater.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/ApiService.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Utils/HashUtils.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <system_error>

SPF_NS_BEGIN
namespace System {

namespace {
std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}
}  // namespace

SelfUpdater::SelfUpdater(ApiService& apiService) : m_apiService(apiService) {}

void SelfUpdater::ApplyPatchAsync(const UpdateInfo& info) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SelfUpdater");

  if (m_busy) {
    logger->Warn("Patch apply already in progress, ignoring new request.");
    return;
  }

  // All exit paths below deliver their outcome through PollResult().
  m_busy = true;
  m_lastResult.reset();
  m_targetVersion = info.latestVersion.full;

  // If the on-disk DLL already matches the patch binary, the update was applied
  // earlier and is just waiting for a restart.
  const auto dllPath = PathManager::GetFrameworkDllPath();
  const auto currentMd5 = ToLower(Utils::HashUtils::CalculateFileMD5(dllPath));
  const auto expectedMd5 = ToLower(info.md5.binary);
  if (!currentMd5.empty() && currentMd5 == expectedMd5) {
    logger->Info("Patch v{} is already applied on disk. Waiting for restart.", m_targetVersion);
    Finish({true, m_targetVersion, ""});
    return;
  }

  if (info.downloadUrl.empty() || expectedMd5.empty()) {
    logger->Error("Patch v{} has incomplete server data (url or md5 missing).", m_targetVersion);
    Finish({false, m_targetVersion, "incomplete_patch_data"});
    return;
  }

  std::error_code ec;
  const auto updateDir = PathManager::GetBasePath() / "update";
  std::filesystem::create_directories(updateDir, ec);
  if (ec) {
    logger->Error("Cannot create update directory {}: {}", updateDir.string(), ec.message());
    Finish({false, m_targetVersion, "update_dir_failed"});
    return;
  }

  m_tempFilePath = updateDir / "spf-framework.dll.new";
  m_expectedMd5 = expectedMd5;
  m_downloadFuture = m_apiService.DownloadFileAsync(info.downloadUrl, m_tempFilePath);
}

PatchApplyResult SelfUpdater::Finalize(const FileDownloadResult& download) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SelfUpdater");
  PatchApplyResult result;
  result.version = m_targetVersion;

  if (!download.success) {
    result.errorMessage = download.errorMessage;
    logger->Error("Patch v{} download failed: {}", m_targetVersion, download.errorMessage);
    return result;
  }

  // Integrity check before touching anything.
  const auto downloadedMd5 = ToLower(Utils::HashUtils::CalculateFileMD5(m_tempFilePath));
  if (downloadedMd5.empty()) {
    result.errorMessage = "md5_calculation_failed";
    logger->Error("Failed to calculate MD5 of the downloaded patch file.");
    return result;
  }
  if (downloadedMd5 != m_expectedMd5) {
    result.errorMessage = "md5_mismatch";
    logger->Error("Patch v{} MD5 mismatch (expected {}, got {}).", m_targetVersion, m_expectedMd5, downloadedMd5);
    std::error_code rmEc;
    std::filesystem::remove(m_tempFilePath, rmEc);
    return result;
  }

  // Swap: the loaded DLL cannot be deleted but CAN be renamed; the backup is
  // removed on the next startup by StartupCleanup().
  const auto dllPath = PathManager::GetFrameworkDllPath();
  const auto backupPath = dllPath.string() + ".old";

  std::error_code ec;
  std::filesystem::rename(dllPath, backupPath, ec);
  if (ec) {
    result.errorMessage = "rename_current_failed";
    logger->Error("Cannot rename current DLL to {}: {}", backupPath, ec.message());
    return result;
  }

  ec.clear();
  std::filesystem::rename(m_tempFilePath, dllPath, ec);
  if (ec) {
    // Roll back so the framework stays functional.
    std::error_code rbEc;
    std::filesystem::rename(backupPath, dllPath, rbEc);
    if (rbEc) logger->Critical("Rollback failed: {} stays as {}. Manual restore required!", dllPath.string(), backupPath);
    result.errorMessage = "place_new_failed";
    logger->Error("Cannot move downloaded patch into place: {}", ec.message());
    return result;
  }

  result.success = true;
  logger->Info("Patch v{} applied successfully. Restart required.", m_targetVersion);
  return result;
}

void SelfUpdater::Finish(const PatchApplyResult& result) { m_lastResult = result; }

std::optional<PatchApplyResult> SelfUpdater::PollResult() {
  if (!m_busy) return std::nullopt;

  // Step 1: download completed -> launch finalize on background thread
  if (m_downloadFuture && m_downloadFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    FileDownloadResult download = m_downloadFuture->get();
    m_downloadFuture.reset();
    m_finalizeFuture = std::async(std::launch::async, [this, download]() { return Finalize(download); });
  }

  // Step 2: finalize completed -> store result
  if (m_finalizeFuture && m_finalizeFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    Finish(m_finalizeFuture->get());
    m_finalizeFuture.reset();
  }

  if (m_lastResult) {
    PatchApplyResult out = *m_lastResult;
    m_lastResult.reset();
    m_busy = false;
    return out;
  }
  return std::nullopt;
}

void SelfUpdater::StartupCleanup() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SelfUpdater");

  std::error_code ec;

  const auto oldDll = PathManager::GetFrameworkDllPath().string() + ".old";
  if (std::filesystem::exists(oldDll, ec)) {
    std::filesystem::remove(oldDll, ec);
    if (ec) {
      logger->Error("Failed to delete old framework backup {}: {}", oldDll, ec.message());
    } else {
      logger->Info("Deleted old framework backup {}.", oldDll);
    }
    ec.clear();
  }

  const auto updateDir = PathManager::GetBasePath() / "update";
  if (std::filesystem::exists(updateDir, ec)) {
    std::filesystem::remove_all(updateDir, ec);
    if (ec) {
      logger->Error("Failed to clean update temp directory {}: {}", updateDir.string(), ec.message());
    } else {
      logger->Info("Cleaned update temp directory.");
    }
  }
}

}  // namespace System
SPF_NS_END
