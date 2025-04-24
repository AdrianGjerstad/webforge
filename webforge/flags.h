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
// File: flags.h
// -----------------------------------------------------------------------------
//
// This header file contains ABSL_DECLARE_FLAG statements for every flag that
// WebForge accepts. This header is meant to be included in any file that needs
// access to the command line flags after they were parsed. Namely, this
// includes the files that implement each subcommand.
//

#ifndef WEBFORGE_FLAGS_H_
#define WEBFORGE_FLAGS_H_

#include "absl/flags/declare.h"

// Flags controlling files and file paths
ABSL_DECLARE_FLAG(std::string, cd);
ABSL_DECLARE_FLAG(std::string, out);
ABSL_DECLARE_FLAG(std::string, depout);
ABSL_DECLARE_FLAG(std::string, logfile);

// Flags controlling processing pipelines and their parameters
ABSL_DECLARE_FLAG(bool, render);
ABSL_DECLARE_FLAG(bool, minify);

#endif  // WEBFORGE_FLAGS_H_

