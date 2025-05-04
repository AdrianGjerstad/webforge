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
// File: cgi.cc
// -----------------------------------------------------------------------------
//
// Implements usage of CGI for WebForge applications
//

#include "webforge/serve/cgi.h"

#include <iostream>
#include <memory>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/site/application.h"
#include "webforge/site/http.h"

extern char** environ;

namespace wf {

namespace {

RequestPtr RequestFromCGIEnvironment() {
  std::shared_ptr<Request> req = std::make_shared<Request>();

  // Iterate through every environment variable and pick out the useful info.
  for (char** env = environ; *env; ++env) {
    std::string e(*env);
    absl::string_view key(e);
    absl::string_view value(e);
    auto pos = key.find('=');
    key.remove_suffix(key.size() - pos);
    value.remove_prefix(pos + 1);

    if (key == "HTTPS") {
      req->UsingTLS(true);
    } else if (key == "REQUEST_METHOD") {
      req->Method(value);
    } else if (key == "PATH_INFO") {
      req->Path(value);
    } else if (key == "QUERY_STRING") {
      ParseQueryString(value, req->MutableQuery());
    } else if (key == "SERVER_PROTOCOL") {
      req->Version(value);
    } else if (key == "CONTENT_TYPE") {
      req->Header("Content-Type", value);
    } else if (key == "CONTENT_LENGTH") {
      req->Header("Content-Length", value);
    } else if (key.find("HTTP_") == 0) {
      key.remove_prefix(5);
      std::string key_name(key);
      std::transform(key_name.begin(), key_name.end(), key_name.begin(),
                     [](unsigned char c) {
        if (c == '_') {
          return '-';
        }

        return (char)c;
      });

      req->Header(key_name, value);
    }
  }

  // CGI uses stdin to read request body data
  req->Stream(std::make_shared<std::istream>(std::cin.rdbuf()));
  
  return req;
}

// Supports writing output in a format that CGI webservers will understand.
class CGIWriter : public ResponseWriter {
public:
  CGIWriter() {
    // Nothing to do.
  }

  absl::Status WriteHead(const Response& res) override {
    std::cout << "status: " << res.Status() << std::endl;
    for (auto& it : res.Headers()) {
      std::cout << it.first << ": " << it.second << std::endl;
    }
    
    for (auto& it : res.Cookies()) {
      std::cout << "set-cookie: " << it.second.ToString() << std::endl;
    }

    // Head is finished
    std::cout << std::endl;

    return absl::OkStatus();
  }

  absl::Status WriteChunk(absl::string_view chunk) override {
    std::cout << chunk;
    return absl::OkStatus();
  }

  void End() override {
    // In theory, we could close std::cout, but something about doing that just
    // feels *wrong*. We do, however, need to flush std::cout.
    std::cout << std::flush;
  }
};

}

int ServeCGI(wf::Application* application) {
  RequestPtr req = RequestFromCGIEnvironment();
  ResponsePtr res = Response::FromRequest(*req);

  std::shared_ptr<CGIWriter> writer = std::make_shared<CGIWriter>();
  res->UseWriter(writer);

  absl::Status s = application->Handle(req, res);
  if (!s.ok()) {
    return 1;
  }

  return 0;
}

}

