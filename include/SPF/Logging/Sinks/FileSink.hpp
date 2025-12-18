#pragma once

#include <SPF/Logging/Logger.hpp>
#include <fstream>
#include <string>
#include <filesystem>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Logging::Sinks {

/**
 * @brief A log sink that writes all messages to the specified file.
 */
class FileSink : public Logging::ILogSink {
 public:
  /**
   * @brief Constructs the sink and opens the file for writing.
   * @param filename The path to the log file.
   * @param name The unique name for this sink.
   */
  FileSink(const std::filesystem::path& filename, const std::string& name);

  // Copying is disallowed as it owns a file handle.
  FileSink(const FileSink&) = delete;
  FileSink& operator=(const FileSink&) = delete;

  // The destructor will close the file automatically.
  ~FileSink() override;

  /**
   * @brief Gets the unique name of the sink.
   */
  fmt::string_view GetName() const override;

  /**
   * @brief Implementation of the virtual method. Writes the message to the file.
   * @param msg The complete log message structure.
   */
  void Log(const Logging::LogMessage& msg) override;

 private:
  std::ofstream m_file;
};

}  // namespace Logging::Sinks

SPF_NS_END
