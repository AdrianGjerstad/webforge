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
// File: middleware.h
// -----------------------------------------------------------------------------
//
// The wf::Middleware object is meant to act as a "middle" step in processing a
// request.
//
// For a full, detailed explanation on how they are used, see router.h.
//

#ifndef WEBFORGE_SITE_MIDDLEWARE_H_
#define WEBFORGE_SITE_MIDDLEWARE_H_

#include <filesystem>
#include <functional>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/site/http.h"

namespace wf {

class Middleware {
public:
  using NextFn = std::function<void(absl::Status)>;

  Middleware();

  // Tells the middleware to do its thing with a request.
  //
  // `req` and `res` are std::shared_ptr's pointing at this request's pair of
  // Request+Response objects. `next` is a function to call with an absl::Status
  // when this Middleware is done processing.
  virtual void operator()(RequestPtr req, ResponsePtr res, NextFn next) = 0;
};

// Defines a Middleware that uses an external function.
//
// Use of this class is not recommended, preferring creating your own Middleware
// classes.
class FMiddleware : public Middleware {
public:
  using NextFn = Middleware::NextFn;
  using MiddlewareFn = std::function<void(RequestPtr, ResponsePtr, NextFn)>;

  FMiddleware(MiddlewareFn middleware);

  void operator()(RequestPtr req, ResponsePtr res, NextFn next) override;

private:
  MiddlewareFn middleware_;
};

// Defines a Middleware that supports serving static files from a directory.
//
// This Middleware is mindful that webroot escape attacks exist, and so 404's
// any requests that match the base and have evidence of such attacks.
class StaticMiddleware : public Middleware {
public:
  using NextFn = Middleware::NextFn;

  // Creates a middleware for serving static files.
  //
  // `dir` is relative to the application-wide component path, and defines the
  // directory to take static files from. `base` defines a base URL path that
  // the files should be accessible at. Requests that match beyond the base will
  // have the base prefix removed before appending it to `dir`.
  StaticMiddleware(const std::filesystem::path& dir,
                   absl::string_view base = "/");

  void operator()(RequestPtr req, ResponsePtr res, NextFn next) override;

private:
  std::filesystem::path dir_;
  std::string base_;
};

}

#endif  // WEBFORGE_SITE_MIDDLEWARE_H_

