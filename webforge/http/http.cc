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

#include "webforge/http/http.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <streambuf>
#include <sstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include <nlohmann/json.hpp>

#include "webforge/core/data.pb.h"
#include "webforge/core/renderer.h"
#include "webforge/http/cookie.h"
#include "webforge/http/strings.h"

extern char** environ;

namespace wf {

namespace {

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
  if (method_ == "get" || method_ == "head" || method_ == "options") {
    return absl::FailedPreconditionError(
      absl::StrFormat("no request body allowed for %s request", method_)
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

  if (stream_ == nullptr) {
    return absl::InternalError("stream is null");
  }

  std::size_t length = std::stoul(headers_.at("content-length"));
  std::string s(length, 0);
  stream_->read(s.data(), length);

  ParseQueryString(s, data);

  return absl::OkStatus();
}

absl::Status Request::ParseJSON(nlohmann::json* data) {
  if (method_ == "get" || method_ == "head" || method_ == "options") {
    return absl::FailedPreconditionError(
      absl::StrFormat("no request body allowed for %s request", method_)
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

  if (stream_ == nullptr) {
    return absl::InternalError("stream is null");
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
  version_("http/0.9"), status_(200), charset_("utf-8"), is_head_(false) {
  // Nothing to do.
}

std::shared_ptr<Response> Response::FromRequest(const Request& req) {
  std::shared_ptr<Response> res = std::make_shared<Response>();

  res->version_ = req.Version();
  res->is_head_ = req.Method() == "head";

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

  if (is_head_) {
    // Head requests do not have response bodies.
    return absl::OkStatus();
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

