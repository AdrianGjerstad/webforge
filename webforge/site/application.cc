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
// File: application.cc
// -----------------------------------------------------------------------------
//
// Implements wf::Application.
//

#include "webforge/site/application.h"

#include <filesystem>
#include <memory>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/core/renderer.h"
#include "webforge/http/http.h"
#include "webforge/site/middleware.h"

namespace wf {

Application::Application(const std::filesystem::path& search_path) :
  renderer_(std::make_shared<Renderer>(search_path)) {
  // Nothing to do.
}

void Application::Use(std::unique_ptr<Middleware> mw) {
  router_.Use(std::move(mw));
}

void Application::Use(absl::string_view path, std::unique_ptr<Middleware> mw) {
  router_.Use(path, std::move(mw));
}

void Application::Get(std::unique_ptr<Middleware> mw) {
  router_.Get(std::move(mw));
}

void Application::Get(absl::string_view path, std::unique_ptr<Middleware> mw) {
  router_.Get(path, std::move(mw));
}

void Application::Post(std::unique_ptr<Middleware> mw) {
  router_.Post(std::move(mw));
}

void Application::Post(absl::string_view path, std::unique_ptr<Middleware> mw) {
  router_.Post(path, std::move(mw));
}

void Application::Error(absl::StatusCode code,
                        std::unique_ptr<Middleware> mw) {
  router_.Error(code, std::move(mw));
}

absl::Status Application::Handle(RequestPtr req, ResponsePtr res) {
  res->UseRenderer(renderer_);

  return router_.Handle(req, res);
}

absl::Status Application::operator()(RequestPtr req, ResponsePtr res) {
  return Handle(req, res);
}

}

