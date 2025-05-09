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
// File: mime.cc
// -----------------------------------------------------------------------------
//
// This file defines the implementation of mime-fetching functions.
//

#include "webforge/site/httpstrings.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"

namespace wf {

namespace {

const absl::flat_hash_map<std::string, std::string> mime_types = {
  // Text-based formats
  {"css", "text/css"},
  {"html", "text/html"},
  {"htm", "text/html"},
  {"js", "text/javascript"},
  {"svg", "image/svg+xml"},
  {"txt", "text/plain"},
  {"xml", "text/xml"},
  
  // Image formats
  {"jpeg", "image/jpeg"},
  {"jpg", "image/jpeg"},
  {"png", "image/png"},
  {"webp", "image/webp"},

  // Application formats
  {"gz", "application/gzip"},
  {"json", "application/json"},
  {"pdf", "application/pdf"},
  {"tar", "application/x-tar"},
  {"xz", "application/x-xz"},
  {"zip", "application/zip"},

  // Audio formats
  {"flac", "audio/flac"},
  {"m4a", "audio/mp4"},
  {"mp3", "audio/mpeg"},
  {"oga", "audio/ogg"},
  {"ogg", "audio/ogg"},
  {"wav", "audio/wav"},

  // Video formats
  {"mov", "video/quicktime"},
  {"mp4", "video/mp4"},

  // Default
  {"_default", "application/octet-stream"},
};

}

const std::string& GetMimeType(absl::string_view name) {
  auto pos = name.find_last_of('/');
  if (pos != absl::string_view::npos) {
    name.remove_prefix(pos + 1);
  }

  pos = name.find_last_of('.');
  if (pos == absl::string_view::npos) {
    // No file extension
    return mime_types.at("_default");
  }
  name.remove_prefix(pos + 1);

  std::string ext(name);
  if (mime_types.contains(ext)) {
    return mime_types.at(ext);
  }

  // Did search, found nothing.
  return mime_types.at("_default");
}

}

