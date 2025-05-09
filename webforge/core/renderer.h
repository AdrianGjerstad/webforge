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
// File: renderer.h
// -----------------------------------------------------------------------------
//
// The wf::Renderer class is the centerpiece of WebForge's build pipeline. It
// uses the [Inja](https://github.com/pantor/inja) Template Engine, which is
// "loosely inspired by jinja for python." The original purpose of Jinja2, to
// which it refers, was to render HTML components into complete documents. Inja,
// however, is sufficiently genericized that we can use it on any file type.
//
// The wf::Renderer class acts as a mature wrapper around inja::Environment,
// which manages templates, optional HTML autoescaping, and piping
// automatically. This class is designed to exist for the lifetime of whatever
// program that uses it, as inja::Template objects are cached for future reuse
// (unless explicitly Flush'd).
//

#ifndef WEBFORGE_CORE_RENDERER_H_
#define WEBFORGE_CORE_RENDERER_H_

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include "webforge/core/data.pb.h"

namespace wf {

class Renderer {
public:
  Renderer(const std::filesystem::path& search_path = ".");

  // Renders a component from an input stream using a set of data.
  //
  // The `key` is a unique value used to identify this *specific* root-level
  // component. If two calls to Render using the same `key` are made, the
  // `component` istream is ignored. Otherwise, `component` is the actual Jinja2
  // template code in istream form. If `component` is null, key is interpreted
  // as a path of a file to open within the search path.
  absl::Status Render(absl::string_view key,
                      std::istream* component,
                      const std::vector<wf::proto::Data>& data,
                      std::ostream* output);

  // Same as Render() but HTML-escapes strings
  absl::Status RenderHTML(absl::string_view key,
                          std::istream* component,
                          const std::vector<wf::proto::Data>& data,
                          std::ostream* output);

  void FlushCache();
  
  const std::filesystem::path& SearchPath() const;

private:
  absl::Status ExpandRenderValue(nlohmann::json* json_value,
                                 const wf::proto::RenderValue& value);

  // Generates a nlohmann::json object for use with inja
  absl::Status PopulateRenderPayload(nlohmann::json* payload,
                                     const std::vector<wf::proto::Data>& data);

  // Checks cache to see if template was already parsed, or parses it.
  //
  // This function may fail if the input template source is malformed in any way
  // that causes it to fail parsing. No cache activity will result in errors.
  absl::StatusOr<const inja::Template> CacheHitOrParse(
    absl::string_view key,
    std::istream* is
  );

  inja::Environment env_;

  std::filesystem::path search_path_;
  absl::flat_hash_map<std::string, inja::Template> template_cache_;
};

}

#endif  // WEBFORGE_CORE_RENDERER_H_

