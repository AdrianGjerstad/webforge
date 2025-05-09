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
// File: httpdate.cc
// -----------------------------------------------------------------------------
//
// This file defines all functions for wf::HTTPDate.
//

#include "webforge/site/httpdate.h"

#include <chrono>
#include <filesystem>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"

namespace wf {

HTTPDate::HTTPDate() : absl::Time(absl::Now()) {
  // Nothing to do.
}

absl::StatusOr<HTTPDate> HTTPDate::FromString(absl::string_view s) {
  std::string error;
  absl::Time t;
  bool success = absl::ParseTime("%a, %d %b %Y %H:%M:%S GMT", s, &t, &error);

  if (success) {
    return HTTPDate(t);
  }

  // Format failed for some reason
  return absl::InvalidArgumentError(
    absl::StrFormat("failed to parse time: %s", error)
  );
}

HTTPDate::HTTPDate(absl::Time t) : absl::Time(
    absl::Trunc(t - absl::UnixEpoch(), absl::Seconds(1)) + absl::UnixEpoch()
  ) {
  // Nothing to do.
}

HTTPDate& HTTPDate::operator=(absl::Time t) {
  *this = HTTPDate(t);
  return *this;
}

HTTPDate::HTTPDate(std::filesystem::file_time_type t) : HTTPDate(
    absl::FromChrono(
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        t - std::filesystem::file_time_type::clock::now() +
          std::chrono::system_clock::now()
      )
    )
  ) {
  // Nothing to do.
}

HTTPDate& HTTPDate::operator=(std::filesystem::file_time_type t) {
  *this = HTTPDate(t);
  return *this;
}

HTTPDate& HTTPDate::operator+=(absl::Duration d) {
  d = absl::Trunc(d, absl::Seconds(1));
  absl::Time::operator+=(d);
  return *this;
}

HTTPDate& HTTPDate::operator-=(absl::Duration d) {
  d = absl::Trunc(d, absl::Seconds(1));
  absl::Time::operator-=(d);
  return *this;
}

HTTPDate HTTPDate::operator+(absl::Duration d) const {
  HTTPDate res(*this);
  res += d;
  return res;
}

HTTPDate HTTPDate::operator-(absl::Duration d) const {
  HTTPDate res(*this);
  res -= d;
  return res;
}

std::string HTTPDate::Render() const {
  return absl::FormatTime("%a, %d %b %Y %H:%M:%S GMT", *this,
                          absl::UTCTimeZone());
}

}

