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
// File: cookie.cc
// -----------------------------------------------------------------------------
//
// This file defines exactly what a wf::Cookie does and how it does it.
//

#include "webforge/http/cookie.h"

#include <optional>
#include <sstream>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"

#include "webforge/http/date.h"
#include "webforge/http/strings.h"

namespace wf {

Cookie::Cookie(absl::string_view key, absl::string_view value) :
  key_(key), value_(value), http_only_(false), secure_(false) {
  // Nothing to do.
}

// Current implementation for deleting a cookie is to set Max-Age=0 in the flags
// for the cookie.
//
// For example, a deleted cookie, with proper flags attatched, from the client's
// POV, would look like this:
// Set-Cookie: mycookie=; HttpOnly; Max-Age=0; Secure
Cookie::Cookie(absl::string_view key) : key_(key), value_(""),
  http_only_(false), max_age_(absl::Seconds(0)), secure_(false) {
  // Nothing to do.
}

std::string Cookie::ToString() const {
  std::ostringstream os;
  os << URLEncode(key_, " \t()<>@,;:\\\"/[]?={}", false) << '='
     << URLEncode(value_, " \",;\\", false);

  if (domain_) {
    os << "; Domain=" << domain_.value();
  }

  if (expires_) {
    os << "; Expires=" << expires_.value().Render();
  }

  if (http_only_) {
    os << "; HttpOnly";
  }

  if (max_age_) {
    os << "; Max-Age=" << absl::ToInt64Seconds(max_age_.value());
  }

  if (path_) {
    os << "; Path=" << path_.value();
  }

  if (same_site_) {
    switch (same_site_.value()) {
    case Cookie::SameSitePolicy::kStrict:
      os << "; SameSite=Strict";
      break;
    case Cookie::SameSitePolicy::kLax:
      os << "; SameSite=Lax";
      break;
    case Cookie::SameSitePolicy::kNone:
      os << "; SameSite=None";
      break;
    default:
      break;
    }
  }

  if (secure_) {
    os << "; Secure";
  }

  return os.str();
}

const std::string& Cookie::Key() const {
  return key_;
}

void Cookie::Key(absl::string_view key) {
  key_ = key;
}

const std::string& Cookie::Value() const {
  return value_;
}

void Cookie::Value(absl::string_view value) {
  value_ = value;
}

const std::optional<std::string>& Cookie::Domain() const {
  return domain_;
}

void Cookie::Domain(absl::string_view domain) {
  domain_ = domain;
}

void Cookie::ClearDomain() {
  domain_.reset();
}

const std::optional<wf::HTTPDate>& Cookie::Expires() const {
  return expires_;
}

void Cookie::Expires(wf::HTTPDate expires) {
  expires_ = expires;
}

void Cookie::ClearExpires() {
  expires_.reset();
}

bool Cookie::HttpOnly() const {
  return http_only_;
}

void Cookie::HttpOnly(bool http_only) {
  http_only_ = http_only;
}

const std::optional<absl::Duration>& Cookie::MaxAge() const {
  return max_age_;
}

void Cookie::MaxAge(absl::Duration max_age) {
  max_age_ = max_age;
}

void Cookie::ClearMaxAge() {
  max_age_.reset();
}

const std::optional<std::string>& Cookie::Path() const {
  return path_;
}

void Cookie::Path(absl::string_view path) {
  path_ = path;
}

const std::optional<Cookie::SameSitePolicy>& Cookie::SameSite() const {
  return same_site_;
}

void Cookie::SameSite(Cookie::SameSitePolicy same_site) {
  same_site_ = same_site;
}

void Cookie::ClearSameSite() {
  same_site_.reset();
}

bool Cookie::Secure() const {
  return secure_;
}

void Cookie::Secure(bool secure) {
  secure_ = secure;
}

}

