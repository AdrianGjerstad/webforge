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
// File: application.h
// -----------------------------------------------------------------------------
//
// wf::Application is a wrapper around a number of facilities, namely wf::Router
// and wf::Renderer. It does not, itself, create wf::Request or wf::Response
// instances. Instead, it has its own Handle method which is passed a pointer
// instance of each.
//

#ifndef WEBFORGE_SITE_APPLICATION_H_
#define WEBFORGE_SITE_APPLICATION_H_

#include <filesystem>
#include <memory>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/core/renderer.h"
#include "webforge/site/http.h"
#include "webforge/site/middleware.h"
#include "webforge/site/router.h"

namespace wf {

class Application {
public:
  // Creates an Application with components inside the specified path.
  //
  // The search_path is passed verbatim to wf::Renderer.
  Application(const std::filesystem::path& search_path = ".");

  void Use(std::unique_ptr<Middleware> mw);
  void Use(absl::string_view path, std::unique_ptr<Middleware> mw);
  void Get(std::unique_ptr<Middleware> mw);
  void Get(absl::string_view path, std::unique_ptr<Middleware> mw);
  void Post(std::unique_ptr<Middleware> mw);
  void Post(absl::string_view path, std::unique_ptr<Middleware> mw);

  // Specifies a handler for when Middleware next's with an error code.
  //
  // See wf::Router::Error
  void Error(absl::StatusCode code, std::unique_ptr<Middleware> mw);

  // Handles a request using the internal Router instance.
  //
  // NOTE: Although wf::Router has a method with this *exact* signature,
  // wf::Application does additional processing before actually routing the
  // request.
  absl::Status Handle(RequestPtr req, ResponsePtr res);

  // Synonym of Handle
  absl::Status operator()(RequestPtr req, ResponsePtr res);

private:
  Router router_;
  std::shared_ptr<Renderer> renderer_;
};

}

#endif  // WEBFORGE_SITE_APPLICATION_H_

