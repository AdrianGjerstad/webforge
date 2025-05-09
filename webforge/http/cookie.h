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
// File: cookie.h
// -----------------------------------------------------------------------------
//
// Declares the wf::Cookie class, used to create and manipulate HTTP cookies and
// setting them with HTTP headers.
//

#ifndef WEBFORGE_SITE_COOKIE_H_
#define WEBFORGE_SITE_COOKIE_H_

#include <iostream>
#include <optional>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"

#include "webforge/http/date.h"

namespace wf {

class Cookie {
public:
  // Valid values for the SameSite= Set-Cookie flag
  enum class SameSitePolicy {
    kStrict = 1,
    kLax = 2,
    kNone = 3,
  };

  // Defines a cookie to have a set value.
  Cookie(absl::string_view key, absl::string_view value);

  // Defines a cookie to be deleted.
  //
  // The created wf::Cookie has some mechanism in it that, when interpreted by
  // the client, will either directly delete the cookie, or, usually, expire the
  // cookie. For example, Max-Age could be set to 0.
  Cookie(absl::string_view key);

  // Creates a stringified version of the cookie for use in Set-Cookie headers.
  //
  // Both the key and value, while stored unencoded in the object, are
  // URL-encoded with the added stipulation that neither semicolons nor double
  // quotes are allowed. Some API's, especially client-side, may not anticipate
  // this scheme.
  std::string ToString() const;
  friend std::ostream& operator<<(std::ostream& os, const Cookie& cookie) {
    os << cookie.ToString();
    return os;
  }

  const std::string& Key() const;
  void Key(absl::string_view key);

  const std::string& Value() const;
  void Value(absl::string_view value);

  const std::optional<std::string>& Domain() const;
  void Domain(absl::string_view domain);
  void ClearDomain();

  const std::optional<wf::HTTPDate>& Expires() const;
  void Expires(wf::HTTPDate expires);
  void ClearExpires();

  bool HttpOnly() const;
  void HttpOnly(bool http_only);

  const std::optional<absl::Duration>& MaxAge() const;
  void MaxAge(absl::Duration max_age);
  void ClearMaxAge();

  const std::optional<std::string>& Path() const;
  void Path(absl::string_view path);

  const std::optional<SameSitePolicy>& SameSite() const;
  void SameSite(SameSitePolicy same_site);
  void ClearSameSite();

  bool Secure() const;
  void Secure(bool secure);

  // For Abseil's use. Please consider using operator<< or ToString().
  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Cookie& cookie) {
    sink.Append(cookie.ToString());
  }

private:
  std::string key_;
  std::string value_;
  std::optional<std::string> domain_;
  std::optional<wf::HTTPDate> expires_;
  bool http_only_;
  std::optional<absl::Duration> max_age_;
  std::optional<std::string> path_;
  std::optional<SameSitePolicy> same_site_;
  bool secure_;
};

}

#endif  // WEBFORGE_SITE_COOKIE_H_

