// Copyright (C) 2025 Adrian Gjerstad
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
// File: file_log_sink.h
// -----------------------------------------------------------------------------
//
// This header declares an absl::LogSink child class used to log text-messages
// (as if they had appeared on the terminal) to a file. This method is used as
// opposed to RecordIO+protobuf because these logs are meant far more for
// troubleshooting build errors than anything else.
//

#ifndef WEBFORGE_FILE_LOG_SINK_H_
#define WEBFORGE_FILE_LOG_SINK_H_

#include <fstream>

#include "absl/log/log.h"
#include "absl/log/log_sink.h"

namespace wf {

// Implements an absl::LogSink that writes log messages to a file.
class FileLogSink : public absl::LogSink {
public:
  // Creates a FileLogSink.
  //
  // filename can be any valid path. If it is a string of size zero, then the
  // LogSink does not actually do anything.
  explicit FileLogSink(const std::string& filename);

  // Provides a way for callers to know if a sink is in an error state.
  bool IsReady() const;

  void Send(const absl::LogEntry& entry) override;

  void Flush() override;

private:
  std::ofstream file_;
};

// Establishes RAII ownership rules when working with wf::FileLogSink.
class FileLogSinkOwner {
public:
  explicit FileLogSinkOwner(const std::string& filename);
  ~FileLogSinkOwner();

  // Provides a way for callers to know if a sink is in an error state.
  bool IsReady() const;

private:
  FileLogSink* sink_;
};

}

#endif  // WEBFORGE_FILE_LOG_SINK_H_

