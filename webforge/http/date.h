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
// File: httpdate.h
// -----------------------------------------------------------------------------
//
// This file declares wf::HTTPDate, which is a descendant of absl::Time. It
// automatically handles time resolution truncation and has facilities for
// parsing and rendering using the proper HTTP-Date format.
//
// Additionally, there may be circumstances where an HTTPDate must be derived
// from other sources, such as std::filesystem::last_write_time(). wf::HTTPDate
// supports initialization and assignment with these types.
//

#ifndef WEBFORGE_SITE_HTTPDATE_H_
#define WEBFORGE_SITE_HTTPDATE_H_

#include <filesystem>
#include <iostream>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"

namespace wf {

class HTTPDate : public absl::Time {
public:
  // Creates an HTTPDate containing the current time.
  HTTPDate();

  // Creates an HTTPDate from another.
  HTTPDate(const HTTPDate& d) = default;
  HTTPDate& operator=(const HTTPDate& d) = default;

  // Creates an HTTPDate from a rendered time string.
  static absl::StatusOr<HTTPDate> FromString(absl::string_view s);

  // Creates or sets an HTTPDate from an absl::Time
  HTTPDate(absl::Time t);
  HTTPDate& operator=(absl::Time t);

  // Creates or sets an HTTPDate from a std::filesystem::file_time_type
  HTTPDate(std::filesystem::file_time_type t);
  HTTPDate& operator=(std::filesystem::file_time_type t);

  // Assignment
  HTTPDate& operator+=(absl::Duration d);
  HTTPDate& operator-=(absl::Duration d);

  // Math
  HTTPDate operator+(absl::Duration d) const;
  HTTPDate operator-(absl::Duration d) const;
  // absl::Time defines this operator, and it doesn't make sense in this
  // context.
  void operator-(absl::Time t) const = delete;

  // Renders a stringified version using absl::FormatTime
  std::string Render() const;
  friend std::ostream& operator<<(std::ostream& os, const HTTPDate& date) {
    os << date.Render();
    return os;
  }
  
  // For Abseil's use. Please consider using operator<< or ToString().
  template <typename Sink>
  friend void AbslStringify(Sink& sink, const HTTPDate& date) {
    sink.Append(date.Render());
  }
};

}

#endif  // WEBFORGE_SITE_HTTPDATE_H_

