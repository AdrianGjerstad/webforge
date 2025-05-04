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
// File: http.cc
// -----------------------------------------------------------------------------
//
// This file implements both wf::Request and wf::Response.
//

#include "webforge/site/http.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <streambuf>
#include <sstream>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include <nlohmann/json.hpp>

#include "webforge/core/data.pb.h"
#include "webforge/core/renderer.h"

extern char** environ;

namespace wf {

namespace {

// Transforms the given string into a consistent casing.
std::string* CaseInsensitive(std::string* s) {
  std::transform(s->begin(), s->end(), s->begin(),
                 [](unsigned char c) { return std::tolower(c); });

  return s;
}

class ResponseStreambuf : public std::streambuf {
public:
  ResponseStreambuf(Response* res) : res_(res) {}

protected:
  std::streamsize xsputn(const char_type* s, std::streamsize n) override {
    res_->Write(std::string(s, n)).IgnoreError();
    return n;
  }

  int_type overflow(int_type c) override {
    if (c != EOF) {
      res_->Write(std::string(1, static_cast<char>(c))).IgnoreError();
    }
    
    return 0;
  }

  int sync() override {
    return 0;
  }

private:
  Response* res_;
};

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

std::string URLDecode(absl::string_view s) {
  std::string result;
  result.reserve(s.size());

  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+') {
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

std::string FormatHTTPDate(absl::Time time) {
  // HTTP Date strings are in the following form:
  // <day>, <date> <month> <year> <hours>:<minutes>:<seconds> <tz>
  //
  // <day> - Three-letter abbreviated day name (Sun, Mon, etc.)
  // <date> - Two-digit day of the month (01, 02, ..., 31)
  // <month> - Three-letter abbreviated month name (Jan, Feb, ..., Dec)
  // <year> - Full year (including century)
  // <hours> - Two-digit hours since midnight
  // <minutes> - Two-digit minutes since the top of the hour
  // <seconds> - Two-digit seconds since the top of the minute
  // <tz> - Name of the timezone, always GMT (times are *never* expressed in
  //        local time)
  return absl::FormatTime("%a, %d %b %Y %H:%M:%S GMT", time,
                          absl::UTCTimeZone());
}

absl::StatusOr<absl::Time> ParseHTTPDate(absl::string_view s) {
  std::string err;
  absl::Time time;
  bool success = absl::ParseTime("%a, %d %b %Y %H:%M:%S GMT", s, &time, &err);
  if (success) {
    return time;
  }

  return absl::InvalidArgumentError(
    absl::StrFormat("invalid HTTP date string: %s", err)
  );
}

absl::Time HTTPTruncateTime(absl::Time time) {
  return absl::Trunc(
    time - absl::UnixEpoch(),
    absl::Seconds(1)
  ) + absl::UnixEpoch();
}

absl::Time FileTimeToAbslTime(std::filesystem::file_time_type ftime) {
  return absl::FromChrono(
    std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ftime - std::filesystem::file_time_type::clock::now() +
        std::chrono::system_clock::now()
    )
  );
}

Cookie::Cookie(absl::string_view key, absl::string_view value) :
  key_(key), value_(value), http_only_(false), secure_(false) {
  // Nothing to do.
}

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
    os << "; Expires=";
    os << absl::FormatTime("%a, %d %b %Y %H:%M:%S GMT",
                           expires_.value(), absl::UTCTimeZone());
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

const std::optional<absl::Time>& Cookie::Expires() const {
  return expires_;
}

void Cookie::Expires(absl::Time expires) {
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

Request::Request() : method_("get"), path_("/"), version_("http/0.9") {
  // Nothing to do.
}

bool Request::UsingTLS() const {
  return using_tls_;
}

void Request::UsingTLS(bool using_tls) {
  using_tls_ = using_tls;
}

const std::string& Request::Method() const {
  return method_;
}

void Request::Method(absl::string_view method) {
  method_ = std::string(method);
  CaseInsensitive(&method_);
}

const std::string& Request::Path() const {
  return path_;
}

void Request::Path(absl::string_view path) {
  path_ = std::string(path);
}

absl::StatusOr<const std::string> Request::Query(absl::string_view key) const {
  std::string key_string(key);
  if (query_.contains(key_string)) {
    return query_.at(key_string);
  }

  return absl::NotFoundError("no query value with that key");
}

void Request::Query(absl::string_view key, absl::string_view value) {
  query_[std::string(key)] = std::string(value);
}

const absl::flat_hash_map<std::string, std::string>& Request::Query() const {
  return query_;
}

absl::flat_hash_map<std::string, std::string>* Request::MutableQuery() {
  return &query_;
}

void Request::ClearQuery() {
  query_.clear();
}

const std::string& Request::Version() const {
  return version_;
}

void Request::Version(absl::string_view version) {
  version_ = std::string(version);
  CaseInsensitive(&version_);
}

absl::StatusOr<const std::string> Request::Header(absl::string_view name) const {
  std::string name_string(name);
  CaseInsensitive(&name_string);
  if (headers_.contains(name_string)) {
    return headers_.at(name_string);
  }

  return absl::NotFoundError("no header with that name");
}

void Request::Header(absl::string_view name, absl::string_view value) {
  std::string name_string(name);
  CaseInsensitive(&name_string);

  if (name_string == "cookie") {
    // We must treat cookies specially. This is not just a regular header.
    std::vector<absl::string_view> parts = absl::StrSplit(value, ';');
    for (auto& it : parts) {
      absl::string_view key(it);
      absl::string_view data(it);
      auto pos = it.find('=');
      if (pos == absl::string_view::npos) {
        data = "1";
      } else {
        data.remove_prefix(pos + 1);
        key.remove_suffix(key.size() - pos);
      }

      // Strip leading and trailing spaces
      pos = key.find_first_not_of(' ');
      if (pos != absl::string_view::npos) {
        key.remove_prefix(pos);
      }
      pos = data.find_last_not_of(' ');
      if (pos != absl::string_view::npos) {
        data.remove_suffix(data.size() - pos - 1);
      }

      cookies_[URLDecode(key)] = URLDecode(data);
    }

    return;
  }

  headers_[name_string] = std::string(value);
}

const absl::flat_hash_map<std::string, std::string>& Request::Headers() const {
  return headers_;
}

absl::flat_hash_map<std::string, std::string>* Request::MutableHeaders() {
  return &headers_;
}

void Request::ClearHeaders() {
  headers_.clear();
}

absl::StatusOr<const std::string> Request::Cookie(absl::string_view name) const {
  std::string name_string(name);
  CaseInsensitive(&name_string);
  if (cookies_.contains(name_string)) {
    return cookies_.at(name_string);
  }

  return absl::NotFoundError("no cookie with that name");
}

void Request::Cookie(absl::string_view name, absl::string_view value) {
  std::string name_string(name);
  CaseInsensitive(&name_string);
  cookies_[name_string] = std::string(value);
}

const absl::flat_hash_map<std::string, std::string>& Request::Cookies() const {
  return cookies_;
}

absl::flat_hash_map<std::string, std::string>* Request::MutableCookies() {
  return &cookies_;
}

void Request::ClearCookies() {
  cookies_.clear();
}

const std::shared_ptr<std::istream>& Request::Stream() const {
  return stream_;
}

std::shared_ptr<std::istream> Request::MutableStream() {
  return stream_;
}

void Request::Stream(std::shared_ptr<std::istream> stream) {
  stream_ = stream;
}

absl::Status Request::ParseURLEncoded(
  absl::flat_hash_map<std::string, std::string>* data) {
  if (method_ == "GET" || method_ == "HEAD" || method_ == "OPTIONS") {
    return absl::FailedPreconditionError(
      absl::StrFormat("no request body allowed for %s", method_)
    );
  }
  
  if (headers_.contains("content-type")) {
    if (headers_.at("content-type") != "application/x-www-form-urlencoded") {
      return absl::FailedPreconditionError(
        absl::StrFormat("incorrect Content-Type for URLEncoded: '%s'",
                        headers_.at("content-type"))
      );
    }
  } else {
    return absl::FailedPreconditionError(
      "no Content-Type header for URLEncoded"
    );
  }

  if (!headers_.contains("content-length")) {
    return absl::FailedPreconditionError("no Content-Length header");
  }

  std::size_t length = std::stoul(headers_.at("content-length"));
  std::string s(length, 0);
  stream_->read(s.data(), length);

  ParseQueryString(s, data);

  return absl::OkStatus();
}

absl::Status Request::ParseJSON(nlohmann::json* data) {
  if (method_ == "GET" || method_ == "HEAD" || method_ == "OPTIONS") {
    return absl::FailedPreconditionError(
      absl::StrFormat("no request body allowed for %s", method_)
    );
  }
  
  if (headers_.contains("content-type")) {
    if (headers_.at("content-type") != "application/json") {
      return absl::FailedPreconditionError(
        absl::StrFormat("incorrect Content-Type for JSON: '%s'",
                        headers_.at("content-type"))
      );
    }
  } else {
    return absl::FailedPreconditionError("no Content-Type header for JSON");
  }

  try {
    *data = nlohmann::json::parse(*stream_);
  } catch (nlohmann::json::exception& e) {
    return absl::InvalidArgumentError(
      absl::StrFormat("json parsing failed: %s", e.what())
    );
  }

  return absl::OkStatus();
}

ResponseWriter::ResponseWriter() {
  // Nothing to do.
}

absl::Status ResponseWriter::WriteEnd(absl::string_view chunk) {
  absl::Status s = WriteChunk(chunk);
  End();

  return s;
}

Response::Response() : head_written_(false), finished_(false),
  version_("http/0.9"), status_(200), charset_("utf-8") {
  // Nothing to do.
}

std::shared_ptr<Response> Response::FromRequest(const Request& req) {
  std::shared_ptr<Response> res = std::make_shared<Response>();

  res->version_ = req.Version();

  return res;  
}

void Response::UseWriter(std::shared_ptr<ResponseWriter> writer) {
  writer_ = writer;
}

void Response::UseRenderer(std::shared_ptr<Renderer> renderer) {
  renderer_ = renderer;
}

bool Response::HeadWritten() const {
  return head_written_;
}

bool Response::Finished() const {
  return finished_;
}

const std::string& Response::Version() const {
  return version_;
}

void Response::Version(absl::string_view version) {
  version_ = std::string(version);
  CaseInsensitive(&version_);
}

int Response::Status() const {
  return status_;
}

void Response::Status(int status) {
  status_ = status;
}

absl::StatusOr<const std::string> Response::Header(absl::string_view name) const {
  std::string name_string(name);
  CaseInsensitive(&name_string);
  if (headers_.contains(name_string)) {
    return headers_.at(name_string);
  }

  return absl::NotFoundError("no header with that name");
}

void Response::Header(absl::string_view name, absl::string_view value) {
  std::string name_string(name);
  CaseInsensitive(&name_string);
  headers_[name_string] = std::string(value);
}

const absl::flat_hash_map<std::string, std::string>& Response::Headers() const {
  return headers_;
}

absl::flat_hash_map<std::string, std::string>* Response::MutableHeaders() {
  return &headers_;
}

void Response::ClearHeaders() {
  headers_.clear();
}

const std::string& Response::Charset() const {
  return charset_;
}

void Response::Charset(absl::string_view charset) {
  charset_ = std::string(charset);
  CaseInsensitive(&charset_);
}

absl::StatusOr<const Cookie> Response::Cookie(absl::string_view name) const {
  if (cookies_.contains(name)) {
    return cookies_.at(name);
  }

  return absl::NotFoundError("no cookie with that name");
}

Cookie* Response::Cookie(absl::string_view name, absl::string_view value) {
  cookies_.emplace(name, wf::Cookie(name, value));
  return &(cookies_.at(name));
}

Cookie* Response::DeleteCookie(absl::string_view name) {
  cookies_.emplace(name, wf::Cookie(name));
  return &(cookies_.at(name));
}

const absl::flat_hash_map<std::string, Cookie>& Response::Cookies() const {
  return cookies_;
}

absl::flat_hash_map<std::string, Cookie>* Response::MutableCookies() {
  return &cookies_;
}

void Response::ClearCookies() {
  cookies_.clear();
}

const std::filesystem::path& Response::ComponentPath() const {
  return renderer_->SearchPath();
}

absl::Status Response::Error() const {
  return error_;
}

void Response::Error(absl::Status s) {
  error_ = s;
}

absl::Status Response::WriteHead() {
  if (head_written_ || finished_) {
    return absl::OkStatus();
  }

  if (!writer_) {
    return absl::UnavailableError("response writer not set");
  }
  
  if (!headers_.contains("content-type")) {
    headers_.emplace("content-type",
                     absl::StrFormat("application/octet-stream; charset=%s",
                                     charset_));
  } else {
    headers_["content-type"] =
      absl::StrFormat("%s; charset=%s", headers_["content-type"], charset_);
  }

  absl::Status s = writer_->WriteHead(*this);
  if (!s.ok()) {
    return s;
  }

  head_written_ = true;
  return absl::OkStatus();
}

absl::Status Response::Write(absl::string_view data) {
  if (finished_) {
    return absl::FailedPreconditionError(
      "cannot write data, response already finished"
    );
  }

  if (!head_written_) {
    absl::Status s = WriteHead();
    if (!s.ok()) {
      return s;
    }
  }

  return writer_->WriteChunk(data);
}

absl::Status Response::End(absl::string_view data) {
  if (!head_written_) {
    // We can actually write the content-length too.
    Header("Content-Length", std::to_string(data.size()));
  }

  absl::Status s = Write(data);
  End();
  return s;
}

void Response::End() {
  if (finished_) {
    return;
  }

  if (!head_written_) {
    WriteHead().IgnoreError();
  }

  writer_->End();
}

absl::Status Response::Render(absl::string_view component,
                              const std::vector<wf::proto::Data>& data) {
  const std::string& mime_type = GetMimeType(component);
  Header("Content-Type", mime_type);

  absl::Status s = WriteHead();
  if (!s.ok()) {
    End();
    return s;
  }

  ResponseStreambuf sb(this);
  std::ostream os(&sb);

  if (mime_type == "text/html") {
    s = renderer_->RenderHTML(std::string(component), nullptr, data, &os);
  } else {
    s = renderer_->Render(std::string(component), nullptr, data, &os);
  }

  return s;
}

}

