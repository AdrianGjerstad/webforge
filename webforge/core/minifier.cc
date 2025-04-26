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
// File: minifier.cc
// -----------------------------------------------------------------------------
//
// The wf::Minifier class manages minification of all three primary web
// languages using a NodeJS subprocess running custom code. This code `require`s
// `html-minifier` and uses two IPC channels to communicate with WebForge. The
// protocol they use is a request-response style protocol described below.
//
// When WebForge wants to have a stream of text minified, the first thing sent
// is the type of source that this text is, represented as a uint8. Next,
// WebForge writes the length of the input source as a uint64. Lastly, it writes
// the actual input source itself, verbatim. All integer values are in network
// byte order.
//
// After receiving a request and performing the appropriate minification steps,
// the subprocess writes a record to WebForge, which is a uint64 representing
// the length of the minified text, followed by the minified text itself.
//

#include "webforge/core/minifier.h"

#include <endian.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"

namespace wf {

namespace {

// NodeJS source code to be run in a child process
const std::string minifier_src =
  "/* Some ways of installing html-minifier install it in a place node doesn't"
  " * recognize. This is the fix."
  " */"
  "module.paths.push('/usr/local/lib/node_modules');"
  "let minify = require('html-minifier').minify;"
  "let fs = require('fs');"
  ""
  "let ipcInput = fs.createReadStream(null, {"
  "  fd: Number(process.env.REQUEST_FD)"
  "});"
  "let ipcOutput = fs.createWriteStream(null, {"
  "  fd: Number(process.env.RESPONSE_FD)"
  "});"
  ""
  "let data = Buffer.allocUnsafe(0);"
  "let state = {type:0,size:0,data:Buffer.allocUnsafe(0)};"
  ""
  "let minifyAndResetState = () => {"
  "  let result = '';"
  "  try {"
  "    if (state.type === 1) {  /* SourceType::kHtml */"
  "      result = minify(state.data.toString(), {"
  "        collapseWhitespace: true,"
  "        removeComments: true,"
  "        removeRedundantAttributes: true,"
  "        removeScriptTypeAttributes: true,"
  "        removeTagWhitespace: true,"
  "        minifyCSS: true,"
  "        minifyJS: true,"
  "      });"
  "    } else if (state.type === 2) {  /* SourceType::kCss */"
  "      result = minify('<style>' + state.data.toString() + '</style>', {"
  "        collapseWhitespace: true,"
  "        removeComments: true,"
  "        removeRedundantAttributes: true,"
  "        removeScriptTypeAttributes: true,"
  "        removeTagWhitespace: true,"
  "        minifyCSS: true,"
  "      });"
  ""
  "      result = result.substr(7, result.length - 15);"
  "    } else if (state.type === 3) {  /* SourceType::kJavaScript */"
  "      result = minify('<script>' + state.data.toString() + '</script>', {"
  "        collapseWhitespace: true,"
  "        removeComments: true,"
  "        removeRedundantAttributes: true,"
  "        removeScriptTypeAttributes: true,"
  "        removeTagWhitespace: true,"
  "        minifyJS: true,"
  "      });"
  ""
  "      result = result.substr(8, result.length - 17);"
  "    } else if (state.type === 4) {  /* SourceType::kXml */"
  "      result = minify(state.data.toString(), {"
  "        collapseWhitespace: true,"
  "        removeComments: true,"
  "        removeTagWhitespace: true,"
  "        keepClosingSlash: true,"
  "        html5: false,"
  "      });"
  "    }"
  "  } catch(e) {}"
  ""
  "  result = Buffer.from(result);"
  "  let size = Buffer.allocUnsafe(8);"
  "  size.writeBigUInt64BE(BigInt(result.length));"
  "  ipcOutput.write(Buffer.concat([size, result]));"
  ""
  "  state.type = 0;"
  "  state.size = 0;"
  "  state.data = Buffer.allocUnsafe(0);"
  "};"
  ""
  "ipcInput.on('data', (chunk) => {"
  "  data = Buffer.concat([data, chunk]);"
  "  if (state.type === 0) {"
  "    if (data.length >= 9) {"
  "      state.type = data.readUInt8(0);"
  "      state.size = Number(data.readBigUInt64BE(1));"
  "      state.data = data.slice(9, 9 + state.size);"
  "      if (state.data.length == state.size) {"
  "        data = data.slice(9 + state.size);"
  "        minifyAndResetState();"
  "      }"
  "    }"
  "  } else {"
  "    let prevDataLength = state.data.length;"
  "    state.data = Buffer.concat([state.data,"
  "                                data.slice(0, state.size -"
  "                                              state.data.length)]);"
  "    if (state.data.length == state.size) {"
  "      data = data.slice(state.size - prevDataLength);"
  "      minifyAndResetState();"
  "    }"
  "  }"
  "});";

}

Minifier::Minifier() : worker_pid_(0), request_file_(nullptr),
                       response_file_(nullptr) {
  // In order to expose potential errors in StartWorkerProcess, that function is
  // called, at the earliest, in Minify.
}

Minifier::~Minifier() {
  if (request_file_ != nullptr) {
    fclose(request_file_);  // Don't care about errors
    request_file_ = nullptr;
  }

  if (response_file_ != nullptr) {
    fclose(response_file_);  // Don't care about errors
    response_file_ = nullptr;
  }

  // Only fails if the process doesn't exist to begin with
  TerminateWorkerProcess().IgnoreError();
}

absl::Status Minifier::Minify(SourceType src_type,
                              std::istream* is,
                              std::ostream* output) {
  absl::Status s = StartWorkerProcess();
  if (!s.ok() && s.code() != absl::StatusCode::kAlreadyExists) {
    return s;
  }

  absl::MutexLock lock(&minify_processing_mutex_);

  std::string src(std::istreambuf_iterator<char>(*is), {});
  uint64_t size = htobe64((uint64_t)src.size());

  uint8_t type = (uint8_t)src_type;

  while(fwrite(&type, sizeof(uint8_t), 1, request_file_) != 1) {
    if (ferror(request_file_)) {
      return absl::AbortedError("failed to write SourceType to worker");
    }
  }
  size_t offset = 0;
  while((offset += fwrite((char*)(&size) + offset,
                          1, sizeof(uint64_t) - offset,
                          request_file_)) != sizeof(uint64_t)) {
    if (ferror(request_file_)) {
      return absl::AbortedError("failed to write size to worker");
    }
  }
  offset = 0;
  while((offset += fwrite(src.c_str() + offset,
                          sizeof(char), src.size() - offset,
                          request_file_)) != src.size()) {
    if (ferror(request_file_)) {
      return absl::AbortedError("failed to write input data to worker");
    }
  }
  fflush(request_file_);

  if (worker_pid_ == 0) {
    return absl::AbortedError("worker died unexpectedly");
  } else if(kill(worker_pid_, 0) != 0) {
    worker_pid_ = 0;
    return absl::AbortedError("worker died unexpectedly");
  }

  // Wait for response
  size = 0;
  offset = 0;
  while ((offset += fread((char*)(&size) + offset,
                          1, sizeof(size) - offset,
                          response_file_)) != sizeof(size)) {
    if (ferror(response_file_)) {
      return absl::AbortedError("error while reading response size");
    }
  }

  size = be64toh(size);
  offset = 0;
  size_t buffer_size = 1024;
  if (size < 1024) {
    buffer_size = size;
  }
  char buffer[1024];
  while ((buffer_size = fread(buffer, 1, buffer_size, response_file_))) {
    if (ferror(response_file_)) {
      return absl::AbortedError("error while reading response data");
    }

    *output << std::string(buffer, buffer_size);

    offset += buffer_size;

    buffer_size = 1024;
    if (size - offset < 1024) {
      buffer_size = size - offset;
    }

    if (offset == size) {
      break;
    }
  }
  
  if (ferror(response_file_)) {
    return absl::AbortedError("error while reading response data");
  }

  return absl::OkStatus();
}

absl::Status Minifier::StartWorkerProcess() {
  if (worker_pid_ != 0 && kill(worker_pid_, 0) == 0) {
    return absl::AlreadyExistsError("worker process already running");
  }

  // Set up pipes for IPC
  int request_fds[2];
  int response_fds[2];

  int res = pipe(request_fds);
  if (res < 0) {
    return absl::ErrnoToStatus(errno, "failed to create request pipe() pair");
  }
  
  res = pipe(response_fds);
  if (res < 0) {
    close(request_fds[0]);
    close(request_fds[1]);
    return absl::ErrnoToStatus(errno, "failed to create response pipe() pair");
  }

  // Pipe FD pairs come as follows: [0] => R, [1] => W

  worker_pid_ = fork();
  if (worker_pid_ < 0) {
    worker_pid_ = 0;
    return absl::ErrnoToStatus(errno, "fork() failed");
  }

  if (worker_pid_ > 0) {
    // Parent process
    close(request_fds[0]);
    close(response_fds[1]);

    request_file_ = fdopen(request_fds[1], "w");
    if (request_file_ == nullptr) {
      return absl::ErrnoToStatus(errno, "fdopen() failed for request pipe");
    }
    
    response_file_ = fdopen(response_fds[0], "r");
    if (response_file_ == nullptr) {
      return absl::ErrnoToStatus(errno, "fdopen() failed for response pipe");
    }

    return absl::OkStatus();
  }

  // Child process: exec
  close(request_fds[1]);
  close(response_fds[0]);
  setenv("REQUEST_FD", std::to_string(request_fds[0]).c_str(), 1);
  setenv("RESPONSE_FD", std::to_string(response_fds[1]).c_str(), 1);

  const char* argv[] = {
    "node", "-e", minifier_src.c_str(), NULL
  };
  res = execv("/usr/bin/node", (char* const*)argv);

  // Something went wrong :(
  // We should probably not fail silently...
  std::cerr << "exec failed for worker process with errno " << errno <<
    std::endl;
  exit(1);
}

absl::Status Minifier::TerminateWorkerProcess() {
  if (worker_pid_ == 0) {
    return absl::NotFoundError("no worker running");
  }

  int res = kill(worker_pid_, SIGTERM);
  if (res != 0) {
    return absl::ErrnoToStatus(errno, "kill() worker failed");
  }

  worker_pid_ = 0;
  return absl::OkStatus();
}

}

