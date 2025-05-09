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
// File: http.h
// -----------------------------------------------------------------------------
//
// This file defines four types: wf::Request, wf::Response, wf::RequestPtr, and
// wf::ResponsePtr. The former two represent request and response objects that
// a WebForge site uses to handle requests, respectively. The latter two are
// aliases for shared_ptr's to the associated object, meant as a shorthand when
// writing application frontend servers.
//

#ifndef WEBFORGE_SITE_HTTP_H_
#define WEBFORGE_SITE_HTTP_H_

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include <nlohmann/json.hpp>

#include "webforge/core/data.pb.h"
#include "webforge/core/renderer.h"
#include "webforge/site/cookie.h"

namespace wf {

class Request {
public:
  Request();

  bool UsingTLS() const;
  void UsingTLS(bool using_tls);

  const std::string& Method() const;
  void Method(absl::string_view method);

  const std::string& Path() const;
  void Path(absl::string_view path);

  absl::StatusOr<const std::string> Query(absl::string_view key) const;
  void Query(absl::string_view key, absl::string_view value);
  const absl::flat_hash_map<std::string, std::string>& Query() const;
  absl::flat_hash_map<std::string, std::string>* MutableQuery();
  void ClearQuery();

  const std::string& Version() const;
  void Version(absl::string_view version);

  absl::StatusOr<const std::string> Header(absl::string_view name) const;
  void Header(absl::string_view name, absl::string_view value);
  const absl::flat_hash_map<std::string, std::string>& Headers() const;
  absl::flat_hash_map<std::string, std::string>* MutableHeaders();
  void ClearHeaders();

  absl::StatusOr<const std::string> Cookie(absl::string_view name) const;
  void Cookie(absl::string_view name, absl::string_view value);
  const absl::flat_hash_map<std::string, std::string>& Cookies() const;
  absl::flat_hash_map<std::string, std::string>* MutableCookies();
  void ClearCookies();

  const std::shared_ptr<std::istream>& Stream() const;
  std::shared_ptr<std::istream> MutableStream();
  void Stream(std::shared_ptr<std::istream> stream);

  // Parses a URL-encoded request body into an absl::flat_hash_map.
  //
  // This method uses the same ParseQueryString function used to parse incoming
  // URL query strings. This method checks three things:
  // - Method is one that allows for a request body
  // - Content-Type is set and equal to application/x-www-form-urlencoded
  // - Content-Length is set (is directly used)
  //
  // If one of the above checks fails, this method returns an
  // absl::FailedPreconditionError.
  absl::Status ParseURLEncoded(absl::flat_hash_map<std::string, std::string>*
                               data);

  // Parses a JSON request body into a nlohmann::json.
  //
  // This method uses nlohmann::json::parse on the stream itself. This method
  // checks two things (Content-Length, if set, is ignored):
  // - Method is one that allows for a request body
  // - Content-Type is set and equal to application/json
  //
  // If one of the above checks fails, this method returns an
  // absl::FailedPreconditionError. Additionally, if JSON parsing fails, an
  // absl::InvalidArgumentError is returned.
  absl::Status ParseJSON(nlohmann::json* data);

private:
  bool using_tls_;
  std::string method_;  // e.g. GET
  std::string path_;  // e.g. /about
  absl::flat_hash_map<std::string, std::string> query_;  // Parsed automatically
  std::string version_;  // e.g. HTTP/1.1
  absl::flat_hash_map<std::string, std::string> headers_;
  absl::flat_hash_map<std::string, std::string> cookies_;
  std::shared_ptr<std::istream> stream_;
};

class Response;

class ResponseWriter {
public:
  ResponseWriter();

  virtual absl::Status WriteHead(const Response& res) = 0;
  virtual absl::Status WriteChunk(absl::string_view chunk) = 0;
  virtual void End() = 0;

  // Guaranteed to call End() even when returning an error.
  absl::Status WriteEnd(absl::string_view chunk);
};

class Response {
public:
  Response();

  // Create a new response instance using some of the data from the request.
  //
  // DOES NOT SET A RESPONSE WRITER.
  static std::shared_ptr<Response> FromRequest(const Request& req);

  void UseWriter(std::shared_ptr<ResponseWriter> writer);
  void UseRenderer(std::shared_ptr<Renderer> renderer);

  bool HeadWritten() const;
  bool Finished() const;

  const std::string& Version() const;
  void Version(absl::string_view version);

  int Status() const;
  void Status(int status);

  absl::StatusOr<const std::string> Header(absl::string_view name) const;
  void Header(absl::string_view name, absl::string_view value);
  const absl::flat_hash_map<std::string, std::string>& Headers() const;
  absl::flat_hash_map<std::string, std::string>* MutableHeaders();
  void ClearHeaders();

  const std::string& Charset() const;
  void Charset(absl::string_view charset);

  absl::StatusOr<const wf::Cookie> Cookie(absl::string_view name) const;
  wf::Cookie* Cookie(absl::string_view name, absl::string_view value);
  wf::Cookie* DeleteCookie(absl::string_view name);
  const absl::flat_hash_map<std::string, wf::Cookie>& Cookies() const;
  absl::flat_hash_map<std::string, wf::Cookie>* MutableCookies();
  void ClearCookies();

  const std::filesystem::path& ComponentPath() const;

  // Gets and sets the current Status (used inside router error handlers)
  absl::Status Error() const;
  void Error(absl::Status s);

  absl::Status WriteHead();
  absl::Status Write(absl::string_view data);
  absl::Status End(absl::string_view data);
  void End();

  // Calls WriteHead if not already done, and calls End once it is done writing
  // data out.
  absl::Status Render(absl::string_view component,
                      const std::vector<wf::proto::Data>& data);

private:
  bool head_written_;
  bool finished_;
  std::string version_;  // e.g. HTTP/1.1
  int status_;  // e.g. 200
  absl::flat_hash_map<std::string, std::string> headers_;
  std::string charset_; // e.g. utf-8
  absl::flat_hash_map<std::string, wf::Cookie> cookies_;
  std::shared_ptr<ResponseWriter> writer_;
  std::shared_ptr<Renderer> renderer_;
  absl::Status error_;
  bool is_head_;  // Was the request a HEAD request?
};

using RequestPtr = std::shared_ptr<Request>;
using ResponsePtr = std::shared_ptr<Response>;

}

#endif  // WEBFORGE_SITE_HTTP_H_

