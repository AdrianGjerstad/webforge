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
// File: router.cc
// -----------------------------------------------------------------------------
//
// This file implements all HTTP request routing logic.
//

#include "webforge/site/router.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

#include "webforge/http/http.h"
#include "webforge/site/middleware.h"

namespace wf {

Route::Route() {
  // Nothing to do.
}

bool Route::Match(const RequestPtr req) const {
  if (method_ && method_.value() != req->Method()) {
    if (method_.value() == "get" && req->Method() == "head") {
      // Handle HEAD requests as if they were GET requests
      // (wf::Response::Write has logic to prevent writing bodies)
    } else {
      return false;
    }
  }

  if (host_) {
    absl::StatusOr<const std::string> s = req->Header("Host");
    if (!s.ok()) {
      // Host header does not exist, yet this Route requires a specific host.
      // Fail.
      return false;
    }

    if (s.value() != host_.value()) {
      return false;
    }
  }

  if (path_ && path_.value() != req->Path()) {
    return false;
  }

  return true;
}

void Route::RequireMethod(absl::string_view method) {
  method_ = std::string(method);
}

const std::optional<std::string>& Route::Method() const {
  return method_;
}

void Route::ClearMethod() {
  method_.reset();
}

void Route::RequireHost(absl::string_view host) {
  host_ = std::string(host);
}

const std::optional<std::string>& Route::Host() const {
  return host_;
}

void Route::ClearHost() {
  host_.reset();
}

void Route::RequirePath(absl::string_view path) {
  path_ = std::string(path);
}

const std::optional<std::string>& Route::Path() const {
  return path_;
}

void Route::ClearPath() {
  path_.reset();
}

Router::Router() {
  // Nothing to do.
}

void Router::Use(const Route& r, std::unique_ptr<Middleware> mw) {
  routes_.push_back(std::make_pair(r, std::move(mw)));
}

void Router::Use(std::unique_ptr<Middleware> mw) {
  Use(Route(), std::move(mw));
}

void Router::Use(absl::string_view path, std::unique_ptr<Middleware> mw) {
  Route r;
  r.RequirePath(path);
  Use(r, std::move(mw));
}

void Router::Get(std::unique_ptr<Middleware> mw) {
  Route r;
  r.RequireMethod("get");
  Use(r, std::move(mw));
}

void Router::Get(absl::string_view path, std::unique_ptr<Middleware> mw) {
  Route r;
  r.RequireMethod("get");
  r.RequirePath(path);
  Use(r, std::move(mw));
}

void Router::Post(std::unique_ptr<Middleware> mw) {
  Route r;
  r.RequireMethod("post");
  Use(r, std::move(mw));
}

void Router::Post(absl::string_view path, std::unique_ptr<Middleware> mw) {
  Route r;
  r.RequireMethod("post");
  r.RequirePath(path);
  Use(r, std::move(mw));
}

void Router::Error(absl::StatusCode code, std::unique_ptr<Middleware> mw) {
  errors_[code] = std::move(mw);
}

void Router::operator()(RequestPtr req, ResponsePtr res,
                        Middleware::NextFn next) {
  NextFactory(0, req, res, nullptr, next)(absl::OkStatus());
}

absl::Status Router::Handle(RequestPtr req, ResponsePtr res) {
  this->operator()(req, res, [this, req, res](absl::Status s) {
    if (s.ok()) {
      // Something either next'd when it shouldn't have, or we ran off the end
      // of the middleware stack. We should handle this.
      if (!res->HeadWritten()) {
        // Send processing to the NotFound handler, if there is one.
        s = absl::NotFoundError("ran off the end of the middleware stack");
      } else {
        return;
      }
    }

    res->Error(s);

    // Someone wasn't happy with the data they were given.
    if (errors_.contains(s.code())) {
      // Fortunately, we have a way to handle that!
      (*(errors_.at(s.code())))(req, res, [req, res, s](absl::Status s2) {
        if (s2.ok()) {
          return;
        }

        // Double failure, lets put an end to this.
        res->Status(500);
        res->Header("Content-Type", "text/plain");
        res->WriteHead().IgnoreError();
        res->Write("An internal server error occurred:\n- ").IgnoreError();
        res->Write(s.ToString(absl::StatusToStringMode::kWithEverything))
          .IgnoreError();

        res->Write(
          "\n\nAdditionally, while handling the above error, another occurred:\n- "
        ).IgnoreError();
        res->Write(s2.ToString(absl::StatusToStringMode::kWithEverything))
          .IgnoreError();
        res->End("\n").IgnoreError();
      });
    } else {
      // Unfortunately, we don't have a way to handle that.
      res->Status(500);
      res->Header("Content-Type", "text/plain");
      res->WriteHead().IgnoreError();
      res->Write("An internal server error occurred:\n- ").IgnoreError();
      res->Write(s.ToString(absl::StatusToStringMode::kWithEverything))
        .IgnoreError();
      res->End("\n").IgnoreError();
    }
  });

  return absl::OkStatus();
}

Middleware::NextFn Router::NextFactory(std::size_t index, RequestPtr req,
                                       ResponsePtr res,
                                       std::shared_ptr<bool> stack_flag,
                                       Middleware::NextFn old_next) {
  return [this, index, req, res, old_next, stack_flag](absl::Status s) {
    if (!s.ok()) {
      // An error occurred in processing, bail.
      old_next(s);
      return;
    }

    // The purpose of the stack_flag bool is to prevent the call stack from
    // going to the moon. The way it works is that the previous next function
    // created something akin to a CondVar, and set it to true. After calling
    // the middleware, it checks to see if the value has changed. If it has,
    // then it knows that next was called before the middleware returned, i.e.,
    // synchronously. This signals then that we need to continue the for loop.
    // Here, we set the value to false and return if that is the situation we're
    // in.
    if (stack_flag) {
      if (*stack_flag) {
        *stack_flag = false;
        return;
      }
    }

    for (std::size_t i = index; i < routes_.size(); ++i) {
      if (!routes_[i].first.Match(req)) {
        continue;
      }

      // This route matches.
      auto new_stack_flag = std::make_shared<bool>(true);
      (*(routes_[i].second))(req, res, NextFactory(i + 1, req, res,
                                                   new_stack_flag, old_next));
      if (*new_stack_flag) {
        // next has not yet been called, so we should stop trying to run routes.
        return;
      }
    }

    // Done with list of routes. Since wf::Router is just a fancy
    // wf::Middleware, we need to call the original next.
    old_next(absl::OkStatus());
  };
}

}

