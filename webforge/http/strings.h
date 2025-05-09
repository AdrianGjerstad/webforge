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
// File: httpstrings.h
// -----------------------------------------------------------------------------
//
// This file declares a series of helper functions when working with strings in
// web-based contexts. For example, this file declares wf::URLEncode, which is
// an optionally-advanced way of percent-encoding strings that may contain
// characters not suitable for transmission within some medium.
//
// Additionally, this file declares an interface for resolving mime types (e.g.
// "application/json") from various sources, in particular, file extensions. The
// implementation of said functions is in mime.cc.
//

#ifndef WEBFORGE_SITE_HTTPSTRINGS_H_
#define WEBFORGE_SITE_HTTPSTRINGS_H_

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"

namespace wf {

// Parses a string in the form foo=bar&baz=ham into its map form
//
// The above example would convert to {{"foo", "bar"}, {"baz", "ham"}}. This
// function executes URLDecode on both the names and values of each pair. This
// function DOES NOT FILTER BINARY DATA. `query` is not cleared before having
// data added.
void ParseQueryString(absl::string_view s,
                      absl::flat_hash_map<std::string, std::string>* query);

// Emits a string from the data given in the form of a query string.
std::string RenderQueryString(
  const absl::flat_hash_map<std::string, std::string>& query
);

// In addition to the characters in disallowed_chars, bytes from 0-31 and 127
// are automatically escaped, and so is '%'. This function also encodes ' '
// (space) as '+' (plus).
std::string URLEncode(absl::string_view s,
                      absl::string_view disallowed_chars = ":/?#[]@!$&'()*+,;=",
                      bool plus_space = true);

// Decodes all %-escape sequences and decodes '+' to ' ' (space). THIS FUNCTION
// DOES NOT FILTER BINARY DATA.
std::string URLDecode(absl::string_view s);

// Gets the mime type of a file based on the extension
const std::string& GetMimeType(absl::string_view name);

// Transforms the given string into a consistent casing (lowercase).
std::string* CaseInsensitive(std::string* s);

}

#endif  // WEBFORGE_SITE_HTTPSTRINGS_H_

