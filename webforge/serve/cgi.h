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
// File: cgi.h
// -----------------------------------------------------------------------------
//
// CGI, or Common Gateway Interface, is a standard "protocol" specified in IETF
// RFC 3875. It defines a mechanism that allows web servers (technically other
// types of servers as well, but it's not specified how those would interact) to
// serve dynamic content.
//
// wf::ServeCGI is a standalone function that turns the program into a
// standards-compliant CGI program. It uses wf::Application and generates a
// wf::Request based on the current environment, and outputs according to the
// RFC with wf::Response.
//
// The whole process is entirely transparent to the developer, meaning it is
// extremely easy to move from CGI to something else in the future, should they
// want to (and they should), as long as the "something else" supports the same
// level or less concurrency.
//

#ifndef WEBFORGE_SERVE_CGI_H_
#define WEBFORGE_SERVE_CGI_H_

#include "webforge/site/application.h"

namespace wf {

// Starts processing a CGI request based on the current environment.
//
// Intended to be called from main() as `return wf::ServeCGI(...);` - the return
// value is an exit code.
int ServeCGI(wf::Application* application);

}

#endif  // WEBFORGE_SERVE_CGI_H_

