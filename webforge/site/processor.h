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
// File: processor.h
// -----------------------------------------------------------------------------
//
// The wf::Processor object is meant to act as an endpoint for processing a
// request.
//
// For a full, detailed explanation on how they are used, see router.h.
//

#ifndef WEBFORGE_SITE_PROCESSOR_H_
#define WEBFORGE_SITE_PROCESSOR_H_

#include <filesystem>
#include <functional>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

#include "webforge/core/data.pb.h"
#include "webforge/site/http.h"
#include "webforge/site/middleware.h"

namespace wf {

class Processor : public Middleware {
public:
  Processor();

  // Overridden because wf::Processor is just a fancy wf::Middleware.
  //
  // Do NOT over-override this in your own classes.
  void operator()(RequestPtr req, ResponsePtr res,
                  Middleware::NextFn next) override;

  // Tells the processor to process a request.
  //
  // There is no next function here, because a wf::Processor is meant to be the
  // endpoint.
  virtual absl::Status operator()(RequestPtr req, ResponsePtr res) = 0;
};

// Defines a Processor that uses an external function.
class FProcessor : public Processor {
public:
  using ProcessorFn = std::function<absl::Status(RequestPtr, ResponsePtr)>;

  FProcessor(ProcessorFn processor);

  absl::Status operator()(RequestPtr req, ResponsePtr res) override;

private:
  ProcessorFn processor_;
};

// Defines a Processor that sends a static file.
class StaticProcessor : public Processor {
public:
  StaticProcessor(const std::filesystem::path& filename);

  absl::Status operator()(RequestPtr req, ResponsePtr res) override;

private:
  std::filesystem::path filename_;
};

// Defines a Processor that renders a dynamic file (template)
class DynamicProcessor : public Processor {
public:
  using AddDataFn = std::function<void(absl::string_view,
                                       const wf::proto::RenderValue&)>;
  using LoadDataFn = std::function<absl::Status(RequestPtr, ResponsePtr,
                                                AddDataFn)>;

  DynamicProcessor(const std::filesystem::path& filename, LoadDataFn load_data);

  absl::Status operator()(RequestPtr req, ResponsePtr res) override;

private:
  std::filesystem::path filename_;
  LoadDataFn load_data_;
};

}

#endif  // WEBFORGE_SITE_PROCESSOR_H_

