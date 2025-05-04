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
// File: processor.cc
// -----------------------------------------------------------------------------
//
// This file implements every Processor that comes with WebForge.
//

#include "webforge/site/processor.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"

#include "webforge/core/data.pb.h"
#include "webforge/site/http.h"
#include "webforge/site/middleware.h"

namespace wf {

Processor::Processor() {
  // Nothing to do.
}

void Processor::operator()(RequestPtr req, ResponsePtr res,
                           Middleware::NextFn next) {
  absl::Status s = this->operator()(req, res);

  if (!s.ok()) {
    // Follow-up needed, this processor just error'd out.
    next(s);
  }

  // Nothing to do here, the processor did its job!
}

FProcessor::FProcessor(ProcessorFn processor) : processor_(processor) {
  // Nothing to do.
}

absl::Status FProcessor::operator()(RequestPtr req, ResponsePtr res) {
  return processor_(req, res);
}

StaticProcessor::StaticProcessor(const std::filesystem::path& filename) :
  filename_(filename) {
  // Nothing to do.
}

absl::Status StaticProcessor::operator()(RequestPtr req, ResponsePtr res) {
  std::filesystem::path filepath = res->ComponentPath() / filename_;
  uintmax_t file_size;
  std::filesystem::file_time_type file_mtime;

  if (!std::filesystem::exists(filepath)) {
    return absl::InternalError("static file does not exist");
  }

  if (!std::filesystem::is_regular_file(filepath)) {
    return absl::InternalError("static filepath is not a regular file");
  }

  try {
    file_size = std::filesystem::file_size(filepath);
  } catch (...) {
    // An error occurred.
    return absl::InternalError("file_size threw");
  }

  try {
    file_mtime = std::filesystem::last_write_time(filepath);
  } catch (...) {
    return absl::InternalError("last_write_time threw");
  }

  auto mtime = HTTPTruncateTime(FileTimeToAbslTime(file_mtime));

  res->Header("Content-Length", std::to_string(file_size));
  res->Header("Content-Type", GetMimeType(filepath.string()));
  res->Header("Last-Modified", FormatHTTPDate(mtime));

  if (req->Header("If-Modified-Since").status().ok()) {
    absl::StatusOr<absl::Time> s_time =
      ParseHTTPDate(req->Header("If-Modified-Since").value());

    // If the time data is malformed, we don't care. Don't error out here.
    if (s_time.ok()) {
      if (mtime <= s_time.value()) {
        // Client has a cached copy of this file already.
        res->Status(304);
        res->End();
        return absl::OkStatus();
      }
    }
  }

  std::ifstream file(filepath);
  if (!file.is_open()) {
    return absl::InternalError("failed to open static file");
  }

  res->WriteHead().IgnoreError();

  // Read file 4KB at a time.
  const std::size_t buffer_size = 4096;
  std::string buffer(buffer_size, 0x00);
  do {
    file.read(buffer.data(), buffer_size);

    absl::string_view buf_sv(buffer);
    buf_sv.remove_suffix(buffer_size - file.gcount());

    res->Write(buf_sv).IgnoreError();
  } while (!file.eof());

  res->End();

  return absl::OkStatus();
}

DynamicProcessor::DynamicProcessor(const std::filesystem::path& filename,
                                   LoadDataFn load_data) :
  filename_(filename), load_data_(load_data) {
  // Nothing to do.
}

absl::Status DynamicProcessor::operator()(RequestPtr req, ResponsePtr res) {
  std::vector<wf::proto::Data> data;

  absl::Status s = load_data_(req, res, [&data](absl::string_view key,
                              const wf::proto::RenderValue& value) {
    wf::proto::Data entry;
    entry.set_key(key);
    wf::proto::RenderValue* v = entry.mutable_value();
    *v = value;
    data.push_back(entry);
  });

  if (!s.ok()) {
    return s;
  }

  return res->Render(filename_.string(), data);
}

}

