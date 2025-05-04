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
// File: middleware.cc
// -----------------------------------------------------------------------------
//
// This file implements all Middleware classes that come with WebForge.
//

#include "webforge/site/middleware.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"

#include "webforge/site/http.h"

namespace wf {

Middleware::Middleware() {
  // Nothing to do.
}

FMiddleware::FMiddleware(FMiddleware::MiddlewareFn middleware) :
  middleware_(middleware) {
  // Nothing to do.
}

void FMiddleware::operator()(RequestPtr req, ResponsePtr res,
                             FMiddleware::NextFn next) {
  middleware_(req, res, next);
}

StaticMiddleware::StaticMiddleware(const std::filesystem::path& dir,
                                   absl::string_view base) :
  dir_(dir), base_(base) {
  if (base_.back() != '/') {
    base_.push_back('/');
  }
}

void StaticMiddleware::operator()(RequestPtr req, ResponsePtr res,
                                  Middleware::NextFn next) {
  // Make sure base matches
  if (req->Path().find(base_) != 0) {
    // No match
    next(absl::OkStatus());
    return;
  }

  // Base matched!
  absl::string_view truncated_path = req->Path();
  truncated_path.remove_prefix(base_.size());

  auto pos = truncated_path.find_first_not_of('/');
  truncated_path.remove_prefix(pos);

  // Make sure there aren't any pesky .. segments!
  pos = -1;
  while ((pos = truncated_path.find("..", pos + 1))
           != absl::string_view::npos) {
    if (pos >= 1 && truncated_path[pos - 1] != '/') {
      // Character before pos is not a /, so we know this .. isn't in a segment
      // on its own.
      continue;
    }

    if (pos + 2 < truncated_path.size() && truncated_path[pos + 2] != '/') {
      // Same deal here
      continue;
    }

    // This is a .. segment on its own, so we should 404.
    next(absl::NotFoundError("path traversal attempt detected"));
    return;
  }

  // truncated_path has been vetted pretty well so far, but there is one more
  // thing I want to do to be absolutely sure.
  std::filesystem::path path(res->ComponentPath() / dir_ / truncated_path);
  if (path.string().find((res->ComponentPath() / dir_).string()) != 0) {
    // Clearly something went wrong, because the path we want to read is outside
    // of our allowed root directory.
    next(absl::NotFoundError("other path traversal attempt caught"));
    return;
  }

  // Okay, now path is safe.
  uintmax_t file_size;
  std::filesystem::file_time_type file_mtime;

  if (!std::filesystem::exists(path)) {
    // The file doesn't exist, we'll have to next
    next(absl::OkStatus());
    return;
  }

  if (!std::filesystem::is_regular_file(path)) {
    next(absl::OkStatus());
    return;
  }

  try {
    file_size = std::filesystem::file_size(path);
  } catch (...) {
    next(absl::InternalError("file_size threw"));
    return;
  }

  try {
    file_mtime = std::filesystem::last_write_time(path);
  } catch (...) {
    next(absl::InternalError("last_write_time threw"));
    return;
  }

  auto mtime = HTTPTruncateTime(FileTimeToAbslTime(file_mtime));

  res->Header("Content-Length", std::to_string(file_size));
  res->Header("Content-Type", GetMimeType(path.string()));
  res->Header("Last-Modified", FormatHTTPDate(mtime));

  if (req->Header("If-Modified-Since").status().ok()) {
    absl::StatusOr<absl::Time> s_time = ParseHTTPDate(
      req->Header("If-Modified-Since").value());
    
    // If the time data is malformed, we don't care. Don't error out here.
    if (s_time.ok()) {
      if (mtime <= s_time.value()) {
        // Client has a cached copy of this file already.
        res->Status(304);
        res->End();
        return;
      }
    }
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    next(absl::InternalError("failed to open static file"));
    return;
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
}

}

