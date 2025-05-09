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
// File: httpstrings.cc
// -----------------------------------------------------------------------------
//
// This file defines the implementation of everything in httpstrings.h EXCEPT
// mime type handling, which is implemented in mime.cc.
//

#include "webforge/http/strings.h"

#include <sstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

namespace wf {

void ParseQueryString(absl::string_view s,
                      absl::flat_hash_map<std::string, std::string>* query) {
  // Step one, iterate over each key=value pair (separated by '&')
  std::vector<absl::string_view> parts = absl::StrSplit(s, '&');
  for (auto& it : parts) {
    // Step two, split at the first occurrence of '=', if exists
    absl::string_view key(it);
    absl::string_view value(it);
    auto pos = key.find_first_of("=");
    if (pos != absl::string_view::npos) {
      key.remove_suffix(key.size() - pos);
      value.remove_prefix(pos + 1);
    } else {
      // No '=' found, make /?foo equivalent to /?foo=1
      value = "1";
    }

    std::string url_decoded_key = URLDecode(key);

    // Prevent query string pollution. Some attack vectors are achieved through
    // user input being unsanitized on the user's browser, and allowing them,
    // with query pollution, to overwrite previously specified keys.
    if (!query->contains(url_decoded_key)) {
      (*query)[url_decoded_key] = URLDecode(value);
    }
  }
}

std::string RenderQueryString(
  const absl::flat_hash_map<std::string, std::string>& query) {
  std::vector<std::string> strings;
  strings.reserve(query.size());

  for (const auto& it : query) {
    strings.push_back(absl::StrFormat("%s=%s",
                                      URLEncode(it.first),
                                      URLEncode(it.second)));
  }

  return absl::StrJoin(strings, "&");
}

std::string URLEncode(absl::string_view s, absl::string_view disallowed_chars,
                      bool plus_space) {
  std::ostringstream os;

  for (std::size_t i = 0; i < s.size(); ++i) {
    if (plus_space && s[i] == ' ') {
      os << '+';
    } else if (disallowed_chars.find(s[i]) != absl::string_view::npos ||
        (s[i] >= 0 && s[i] <= 31) || (s[i] == 127) || (s[i] == '%')) {
      // Encoding needed
      os << '%';

      char c1 = s[i] >> 4;
      char c2 = s[i] & 0xf;
      
      if (c1 < 10) {
        os << (char)(c1 + '0');
      } else {
        os << (char)(c1 - 10 + 'A');
      }

      if (c2 < 10) {
        os << (char)(c2 + '0');
      } else {
        os << (char)(c2 - 10 + 'A');
      }
    } else {
      os << s[i];
    }
  }

  return os.str();
}

std::string URLDecode(absl::string_view s, bool plus_space) {
  std::string result;
  result.reserve(s.size());

  for (std::size_t i = 0; i < s.size(); ++i) {
    if (plus_space && s[i] == '+') {
      result.push_back(' ');
    } else if (s[i] == '%') {
      if (i + 2 >= s.size()) {
        // Invalid %-encoding: unexpected EOF
        return result;
      }

      unsigned char c = 0;

      // Bear with me. The following loop executes twice, once for each nibble
      // of the byte. This loop is written here to avoid code duplication
      int j = 0;
      for (; j < 2; ++j) {
        c <<= 4;
        ++i;

        if (s[i] >= '0' && s[i] <= '9') {
          c |= s[i] - '0';
        } else if (s[i] >= 'a' && s[i] <= 'f') {
          c |= s[i] - 'a' + 10;
        } else if (s[i] >= 'A' && s[i] <= 'F') {
          c |= s[i] - 'A' + 10;
        } else {
          // Invalid %-encoding: unexpected non-hex character
          if (j == 0) {
            ++i;
          }
          
          j = 0;
          break;
        }
      }

      // Checks if the loop broke prematurely. This only happens if there was an
      // error. If the comparison is true, the loop completed normally.
      if (j != 0) {
        result.push_back(c);
      }
    } else {
      result.push_back(s[i]);
    }
  }

  return result;
}

std::string* CaseInsensitive(std::string* s) {
  std::transform(s->begin(), s->end(), s->begin(),
                 [](unsigned char c) { return std::tolower(c); });

  return s;
}

}

