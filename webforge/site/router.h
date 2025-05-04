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
// File: router.h
// -----------------------------------------------------------------------------
//
// The wf::Router is responsible for collecting processing and middleware
// objects and executing the correct ones in order to handle all incoming HTTP
// requests.
//
// Each wf::Processor and wf::Middleware is stored as a std::unique_ptr owned by
// the wf::Router.
//
// Example use:
//   absl::Status GETIndex(wf::RequestPtr req, wf::ResponsePtr res) {
//     res->Header("Content-Type", "text/plain");
//     res->End("Hello, world!");
//
//     return absl::OkStatus();
//   }
//
//   absl::Status NotFound(wf::RequestPtr req, wf::ResponsePtr res) {
//     res->Status(404);
//     res->Header("Content-Type", "text/plain");
//     res->End("404 Not Found!");
//
//     return absl::OkStatus();
//   }
//
//   int main(int argc, char** argv) {
//     wf::Router router;
//
//     router.Get("/", std::move(std::make_unique<wf::FProcessor>(GETIndex)));
//     router.Use(std::move(std::make_unique<wf::FProcessor>(NotFound)));
//
//     // Do things with router
//     wf::ServeCGI(router);
//   }
//
// In the example above, the process of serving a CGI request goes as follows:
// - The first route (created when Get() was called) is checked for a match with
//   the incoming request. In this case, the equivalent environment variables
//   REQUEST_METHOD, PATH_INFO, and HTTP_HOST are checked. If there is a match,
//   one of two things could happen:
//   (a) For wf::Processor instances: The processor is executed and is left to
//       its own devices. Asynchronous operations can be used, as long as a
//       response is sent eventually.
//   (b) For wf::Middleware instances: The middleware is given a callback when
//       executed. This callback MUST be called when the middleware is done to
//       continue the search for matching middleware and hopefully an eventual
//       processor.
// - If there is no match on the first route, wf::Router continues its search to
//   the next route. The next route was created by Use(), so it does not attempt
//   to match anything, and runs unconditionally.
//
// Note: wf::Router is virtual-host-aware. The above example is just not complex
// enough to show it. Routers can have subrouters added via Path(base, router).
// If `base` starts with a `/`, it is assumed to take on the same virtual host
// of the parent router, or all hosts. Otherwise, the text leading up to the
// first `/` is assumed to be a Host header to be matched. This is also
// supported with Processor and Middleware.
//

#ifndef WEBFORGE_SITE_ROUTER_H_
#define WEBFORGE_SITE_ROUTER_H_

#include <memory>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/site/http.h"
#include "webforge/site/middleware.h"

namespace wf {

// Defines a set of parameters that can be matched against an actual Request.
class Route {
public:
  Route();

  bool Match(const RequestPtr req) const;

  void RequireMethod(absl::string_view method);
  const std::optional<std::string>& Method() const;
  void ClearMethod();

  void RequireHost(absl::string_view method);
  const std::optional<std::string>& Host() const;
  void ClearHost();

  void RequirePath(absl::string_view method);
  const std::optional<std::string>& Path() const;
  void ClearPath();

private:
  std::optional<std::string> method_;
  std::optional<std::string> host_;
  std::optional<std::string> path_;
};

class Router : public Middleware {
public:
  Router();

  void Use(const Route& r, std::unique_ptr<Middleware> mw);

  void Use(std::unique_ptr<Middleware> mw);
  void Use(absl::string_view path, std::unique_ptr<Middleware> mw);
  void Get(std::unique_ptr<Middleware> mw);
  void Get(absl::string_view path, std::unique_ptr<Middleware> mw);
  void Post(std::unique_ptr<Middleware> mw);
  void Post(absl::string_view path, std::unique_ptr<Middleware> mw);

  // Specifies a handler for when Middleware next's with an error code.
  //
  // Each absl::StatusCode that can be entered has only one Middleware slot
  // available. This function *sets* the middleware for each error status.
  // Default behavior for errors without middleware available is to return a
  // text/plain 500 response with the status. Therefore, be careful what
  // messages you put in error statuses when you don't have an error middleware
  // set.
  void Error(absl::StatusCode code, std::unique_ptr<Middleware> mw);

  void operator()(RequestPtr req, ResponsePtr res,
                  Middleware::NextFn next) override;

  // Handles a request by sending a response.
  //
  // Under the hood, this invokes operator()() with a bogus next function.
  absl::Status Handle(RequestPtr req, ResponsePtr res);

private:
  Middleware::NextFn NextFactory(std::size_t index, RequestPtr req,
                                 ResponsePtr res,
                                 std::shared_ptr<bool> stack_flag,
                                 Middleware::NextFn old_next);

  std::vector<std::pair<Route, std::unique_ptr<Middleware>>> routes_;
  absl::flat_hash_map<absl::StatusCode, std::unique_ptr<Middleware>> errors_;
};

}

#endif  // WEBFORGE_SITE_ROUTER_H_

