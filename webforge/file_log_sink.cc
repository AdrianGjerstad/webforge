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
// File: file_log_sink.cc
// -----------------------------------------------------------------------------
//
// This file implements wf::FileLogSink as a logging sink that writes
// text-messages to a file.
//

#include "webforge/file_log_sink.h"

#include <fstream>

#include "absl/log/log.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"

namespace wf {

FileLogSink::FileLogSink(const std::string& filename) {
  if (filename.size() == 0) {
    return;
  }

  file_ = std::ofstream(filename, std::ios::app);

  if (!file_) {
    PLOG(ERROR) << "Failed to create log sink for file " << filename;
  }
}

bool FileLogSink::IsReady() const {
  return file_.is_open();
}

void FileLogSink::Send(const absl::LogEntry& entry) {
  if (file_) {
    file_ << entry.text_message_with_prefix_and_newline();
  }
}

void FileLogSink::Flush() {
  file_.flush();
}

FileLogSinkOwner::FileLogSinkOwner(const std::string& filename)
  : sink_(new FileLogSink(filename)) {
  if (sink_ == nullptr) {
    PLOG(ERROR) << "Failed to create log sink from owner";
  } else {
    absl::AddLogSink(sink_);
  }
}

FileLogSinkOwner::~FileLogSinkOwner() {
  if (sink_ != nullptr) {
    absl::RemoveLogSink(sink_);
    delete sink_;
  }
}

bool FileLogSinkOwner::IsReady() const {
  if (sink_ != nullptr) {
    return sink_->IsReady();
  }

  return false;
}

}

