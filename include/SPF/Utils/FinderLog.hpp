#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Logging/Logger.hpp"

#include "fmt/base.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

SPF_NS_BEGIN

namespace Utils {

class FinderLog {
 public:
  /**
   * @brief One logical stage of offset discovery within a Finder.
   *
   * Phase groups related pattern-search steps together and provides:
   * - Per-step logging (address found, NOT FOUND, offset value, etc.)
   * - Automatic tail summary when phase goes out of scope `[+] PhaseName: 3/4 OK (12ms)`
   * - Accumulation of step results into the owning FinderLog
   *
   * Usage:
   * @code
   *   {
   *     auto p = log.MakePhase("Manager Accessor");
   *     p.Step(addr, "GetManagerAccessor");
   *     p.Step(countAddr, "Count", "DATA");
   *   } // ~Phase emits summary
   * @endcode
   */
  class Phase {
   public:
    /**
     * @brief Move constructor. Transfers phase ownership to a new scope.
     * @param other Source phase — becomes inert (m_log set to nullptr).
     */
    Phase(Phase&& other) noexcept;
    /**
     * @brief Move assignment. Emits tail summary for the current phase first,
     *        then takes over the source phase.
     * @param other Source phase — becomes inert.
     * @return Reference to this.
     */
    Phase& operator=(Phase&& other) noexcept;
    /**
     * @brief Destructor. Emits tail summary and accumulates step results
     *        into the owning FinderLog.
     */
    ~Phase();

    Phase(const Phase&) = delete;
    Phase& operator=(const Phase&) = delete;

    /**
     * @brief Log one critical step: pointer/address search.
     * @param addr Found address (0 = not found).
     * @param desc Human-readable description (e.g. "GetManagerAccessor").
     * @param tag Optional category tag printed in brackets (e.g. "DATA", "NODE").
     * @return true if addr is valid, false otherwise.
     *
     * @details On failure the phase is marked as failed (HasFailure() → true)
     *          and the step is logged at ERROR level with `[x]` marker.
     *          On success it logs at DEBUG level with `[+]` marker.
     */
    bool Step(uintptr_t addr, std::string_view desc, std::string_view tag = {});
    /**
     * @brief Log one optional step: pointer/address search that may fail.
     * @param addr Found address (0 = not found).
     * @param desc Human-readable description.
     * @param tag Optional category tag.
     * @return true if addr is valid, false otherwise.
     *
     * @details Unlike Step(), failure does NOT mark the phase as failed.
     *          Logged at WARN level with `[!]` marker.
     */
    bool StepOptional(uintptr_t addr, std::string_view desc, std::string_view tag = {});
    /**
     * @brief Log one critical step: int32 offset within a struct.
     * @param offset Found offset value.
     * @param desc Human-readable description.
     * @param tag Optional category tag.
     * @return true if offset is sane (IsSaneOffset), false otherwise.
     *
     * @details On failure the phase is marked as failed and logged at ERROR level.
     *          On success logs the offset hex value at DEBUG level.
     */
    bool StepOffset(int32_t offset, std::string_view desc, std::string_view tag = {});
    /**
     * @brief Log one optional step: int32 offset that may be absent.
     * @param offset Found offset value.
     * @param desc Human-readable description.
     * @param tag Optional category tag.
     * @return true if offset is sane, false otherwise.
     *
     * @details Does NOT mark the phase as failed on failure.
     *          Logged at WARN level with `[!]` marker.
     */
    bool StepOffsetOptional(int32_t offset, std::string_view desc, std::string_view tag = {});

    /**
     * @brief Check whether any critical step failed in this phase.
     * @return true if at least one Step() or StepOffset() call failed.
     */
    bool HasFailure() const { return m_hasFailure; }
    /**
     * @brief Total number of steps logged in this phase.
     * @return Step count.
     */
    int Steps() const { return m_stepNum; }
    /**
     * @brief Number of successful steps in this phase.
     * @return Success count.
     */
    int StepsOk() const { return m_stepOk; }

   private:
    friend class FinderLog;
    Phase(FinderLog& log, std::string_view name);

    void EmitTailSummary();
    void EmitStep(int num, uintptr_t addr, bool valid, std::string_view desc, std::string_view tag, bool critical);

    FinderLog* m_log = nullptr;
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;
    int m_stepNum = 0;
    int m_stepOk = 0;
    bool m_hasFailure = false;
  };

  /**
   * @brief Construct a FinderLog for a named finder.
   * @param finderName Logger channel name (e.g. "FileSystemDataFinder").
   *
   * @details Creates a child logger via LoggerFactory, captures module base
   *          address for relative address formatting, and starts the timer.
   */
  explicit FinderLog(std::string_view finderName);
  ~FinderLog() = default;

  FinderLog(const FinderLog&) = delete;
  FinderLog& operator=(const FinderLog&) = delete;

  /**
   * @brief Create a new Phase within this finder's logging session.
   * @param name Phase name printed in the header and summary (e.g. "Manager Accessor").
   * @return A Phase object; its destructor will emit the tail summary.
   */
  Phase MakePhase(std::string_view name);
  /**
   * @brief Finalize the finder log and emit a summary line.
   * @param isReady Whether the finder completed successfully.
   * @return The value of isReady (pass-through for convenience).
   *
   * @details Prints `── FinderName: READY (ok/total, Nms) ──` or
   *          `── FinderName: FAILED (ok/total, Nms) [x] ──`.
   *          Subsequent calls are no-ops.
   */
  bool Finish(bool isReady);

  /** @brief Log at INFO level through the finder's logger channel. */
  template <typename... Args>
  void Info(fmt::string_view f, Args&&... args) {
    m_logger->Info(f, std::forward<Args>(args)...);
  }
  /** @brief Log at DEBUG level through the finder's logger channel. */
  template <typename... Args>
  void Debug(fmt::string_view f, Args&&... args) {
    m_logger->Debug(f, std::forward<Args>(args)...);
  }
  /** @brief Log at WARN level through the finder's logger channel. */
  template <typename... Args>
  void Warn(fmt::string_view f, Args&&... args) {
    m_logger->Warn(f, std::forward<Args>(args)...);
  }
  /** @brief Log at ERROR level through the finder's logger channel. */
  template <typename... Args>
  void Error(fmt::string_view f, Args&&... args) {
    m_logger->Error(f, std::forward<Args>(args)...);
  }

  /**
   * @brief Format an absolute address as a human-readable string relative to the module base.
   * @param addr Absolute address in process memory.
   * @return Formatted string: "0xADDR (module_name +0xOFFSET)" or "(null)" if addr is 0.
   */
  std::string Rel(uintptr_t addr) const;
  /**
   * @brief Validate that an address points to readable memory.
   * @param addr Address to validate.
   * @return true if the address is valid and readable.
   */
  bool ValidateAddr(uintptr_t addr) const;

  /** @brief Name of this finder (logger channel name). */
  const std::string& Name() const { return m_name; }
  /** @brief Underlying logger instance for this finder. */
  std::shared_ptr<Logging::Logger> GetLogger() const { return m_logger; }

 private:
  void InitModuleInfo();

  std::string m_name;
  std::shared_ptr<Logging::Logger> m_logger;
  uintptr_t m_moduleBase = 0;
  std::string m_moduleName;
  std::chrono::steady_clock::time_point m_start;
  int m_totalSteps = 0;
  int m_okSteps = 0;
  bool m_finished = false;
};

}  // namespace Utils
SPF_NS_END
