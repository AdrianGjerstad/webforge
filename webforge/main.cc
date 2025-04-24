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
// WebForge is a command-line CMS, and this file is its entrypoint. It is
// responsible for handling command line flags and invoking the necessary
// underlying library functionality.
//
// WebForge comes in two parts: a core and a CLI. The core is C++ code that is
// meant to be interfaced into by other C++ code. That other C++ code could be
// another custom tool built by someone else, a CGI script, a server, or another
// program. The CLI is a wrapper around that C++ library that enables command
// line usage.
//

#include <unistd.h>

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"

#include "webforge/file_log_sink.h"
#include "webforge/flags.h"

#define WEBFORGE_CLI_VERSION "0.1.0"

ABSL_FLAG(std::string, cd, "components",
          "Specify a 'component directory', or a root path for components");
ABSL_FLAG(std::string, out, "",
          "Specify an output file to dump the compiled output to");
ABSL_FLAG(std::string, depout, "",
          "Specify an output file to dump a dependency list to");
ABSL_FLAG(std::string, logfile, "",
          "Optionally specify a file to dump processing logs to");
ABSL_FLAG(bool, render, true,
          "Specify if the rendering pipeline should be used in processing the "
          "input file");
ABSL_FLAG(bool, minify, true,
          "Specify if the maybe-rendered output should be minified");

namespace {

// Returns a user-ready string with the WebForge version.
//
// If built without -DNDEBUG, or Bazel --compilation_mode=opt, this function
// also exposes that the binary is a debug-mode build.
std::string GetWebForgeVersion() {
  std::string version_str("WebForge CLI version " WEBFORGE_CLI_VERSION);
  
  version_str += '\n';

#ifndef NDEBUG
  version_str += "Debug build (NDEBUG not #defined)\n";
#endif  // NDEBUG

  return version_str;
}

// Provides additional after-the-fact sanitization for CLI flags.
absl::Status SanitizeCommandLineFlags() {
  if (absl::GetFlag(FLAGS_out).size() == 0) {
    LOG(WARNING) << "No output file provided (via --out), using stdout";

    absl::SetFlag(&FLAGS_out, absl::StrFormat("pipe:%d", fileno(stdout)));
  }

  return absl::OkStatus();
}

}

int main(int argc, char** argv) {
  absl::FlagsUsageConfig cfg;
  
  absl::SetProgramUsageMessage("fast and effective command-line CMS");
  cfg.version_string = &GetWebForgeVersion;
  absl::SetFlagsUsageConfig(cfg);
  std::vector<char*> positionals = absl::ParseCommandLine(argc, argv);
 
  absl::InitializeLog();

  wf::FileLogSinkOwner flso(absl::GetFlag(FLAGS_logfile));

  VLOG(1) << "Starting WebForge CLI version " << WEBFORGE_CLI_VERSION;
#ifndef NDEBUG
  LOG(WARNING) << "Running a debug build of WebForge, not meant for production "
                  "use";
#endif  // NDEBUG

  absl::Status s = SanitizeCommandLineFlags();
  if (!s.ok()) {
    LOG(ERROR) << s;
    return 1;
  }

  return 0;
}

