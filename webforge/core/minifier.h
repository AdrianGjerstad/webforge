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
// File: minifier.h
// -----------------------------------------------------------------------------
//
// The wf::Minifier class is a post-processing feature of WebForge that minifies
// HTML, CSS, and JavaScript all with one NodeJS package, html-minifier.
//

#ifndef WEBFORGE_CORE_MINIFIER_H_
#define WEBFORGE_CORE_MINIFIER_H_

#include <stdio.h>
#include <sys/types.h>

#include <iostream>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"

namespace wf {

// NOTE: Changes here must also be made inside minifier_src in minifier.cc.
enum class SourceType {
  kHtml = 1,
  kCss = 2,
  kJavaScript = 3,
};

class Minifier {
public:
  Minifier();
  ~Minifier();

  // Minifies a source text based on its type.
  //
  // First call within this Minifier instance is naturally more expensive than
  // the rest. The implementation requires a worker process running NodeJS to be
  // started in order to minify data.
  absl::Status Minify(SourceType src_type,
                      std::istream* is,
                      std::ostream* output);

private:
  absl::Status StartWorkerProcess();
  absl::Status TerminateWorkerProcess();

  pid_t worker_pid_;
  FILE* request_file_;
  FILE* response_file_;
  absl::Mutex minify_processing_mutex_;
};

}

#endif  // WEBFORGE_CORE_MINIFIER_H_

