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
// File: main.cc
// -----------------------------------------------------------------------------
//
// See the basics of how a CGI program using WebForge works.
//
// This CGI application serves exactly one endpoint:
// / - Serves a plain-text hello-world message.
// All other requests are handled as a 404.
//

#include <memory>

#include "absl/status/status.h"
#include "webforge/http/http.h"
#include "webforge/serve/cgi.h"
#include "webforge/site/application.h"
#include "webforge/site/processor.h"

absl::Status HelloWorld(wf::RequestPtr req, wf::ResponsePtr res) {
  res->Header("Content-Type", "text/plain");
  res->End("Hello, world!\n").IgnoreError();

  return absl::OkStatus();
}

absl::Status NotFoundError(wf::RequestPtr req, wf::ResponsePtr res) {
  res->Status(404);
  res->Header("Content-Type", "text/plain");
  res->Write("404 Not Found\n").IgnoreError();

  if (res->Error().ok()) {
    res->End();
  } else {
    res->Write(res->Error().ToString()).IgnoreError();
    res->End("\n").IgnoreError();
  }

  return absl::OkStatus();
}

int main(int argc, char** argv) {
  wf::Application app;

  // Routes
  app.Get("/", std::move(std::make_unique<wf::FProcessor>(HelloWorld)));

  // Error pages
  // If a Processor returns a non-OK status or a Middleware next's with a non-OK
  // status, a Processor is selected to handle the error based on the provided
  // status. Unhandled errors send 500 codes to the client. Additionally,
  // requests that go unhandled at the end of the processing stack are directed
  // to the NotFound error handler.
  app.Error(absl::StatusCode::kNotFound,
            std::move(std::make_unique<wf::FProcessor>(NotFoundError)));
  return wf::ServeCGI(&app);
}

