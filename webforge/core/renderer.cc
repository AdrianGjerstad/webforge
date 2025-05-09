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
// File: renderer.cc
// -----------------------------------------------------------------------------
//
// This file implements the wf::Renderer class as a caching wrapper around
// inja::Environment.
//

#include "webforge/core/renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include "webforge/core/data.pb.h"

namespace wf {

Renderer::Renderer(const std::filesystem::path& search_path) :
  search_path_(search_path) {
  // By default, we don't want to escape HTML strings. The RenderHTML function
  // interacts with this part of inja, and the rest of the code makes the
  // assumption that strings will not be HTML-escaped.
  env_.set_html_autoescape(false);

  // Tell inja::Environment we want to be in charge of included/extended
  // template lookups.
  env_.set_search_included_templates_in_files(false);

  env_.set_include_callback([this, search_path]
                            (const std::filesystem::path& current_path,
                             const std::string& name) {
    // search_path + name or else throw (I hate throw but it is what it is)
    std::filesystem::path new_path(search_path / name);

    std::ifstream is(new_path.string());
    if (!is.is_open()) {
      throw inja::InjaError("file_error",
        absl::StrFormat("no such template '%s'", name));
    }

    absl::StatusOr<const inja::Template> s_tmpl = CacheHitOrParse(name, &is);
    if (!s_tmpl.ok()) {
      throw inja::InjaError("parser_error",
        std::string(s_tmpl.status().message()));
    }

    return s_tmpl.value();
  });
}

absl::Status Renderer::Render(const std::string& key,
                              std::istream* component,
                              const std::vector<wf::proto::Data>& data,
                              std::ostream* output) {
  absl::StatusOr<const inja::Template> s_tmpl = CacheHitOrParse(key,
                                                                component);
  if (!s_tmpl.ok()) {
    return s_tmpl.status();
  }
  const inja::Template& tmpl = s_tmpl.value();

  nlohmann::json render_payload({});
  absl::Status s = PopulateRenderPayload(&render_payload, data);
  if (!s.ok()) {
    return s;
  }

  try {
    env_.render_to(*output, tmpl, render_payload);
  } catch (const inja::InjaError& e) {
    return absl::AbortedError(std::string("failed to render template: ") +
                              e.what());
  }

  return absl::OkStatus();
}

absl::Status Renderer::RenderHTML(const std::string& key,
                                  std::istream* component,
                                  const std::vector<wf::proto::Data>& data,
                                  std::ostream* output) {
  env_.set_html_autoescape(true);
  absl::Status s = Render(key, component, data, output);
  env_.set_html_autoescape(false);
  return s;
}

void Renderer::FlushCache() {
  // Nice and easy :)
  template_cache_.clear();
}

const std::filesystem::path& Renderer::SearchPath() const {
  return search_path_;
}

absl::Status Renderer::ExpandRenderValue(nlohmann::json* json_value,
                                         const wf::proto::RenderValue& value) {
  if (value.has_text()) {
    *json_value = value.text();
  } else if (value.has_integer()) {
    *json_value = value.integer();
  } else if (value.has_real()) {
    *json_value = value.real();
  } else if (value.has_vector()) {
    std::vector<nlohmann::json> vec;
    for (int i = 0; i < value.vector().vector_size(); ++i) {
      nlohmann::json subvalue;
      absl::Status s = ExpandRenderValue(&subvalue, value.vector().vector(i));
      if (!s.ok()) {
        return s;
      }

      vec.push_back(subvalue);
    }

    *json_value = vec;
  } else {
    return absl::DataLossError("RenderValue did not have any value assigned");
  }

  return absl::OkStatus();
}

absl::Status Renderer::PopulateRenderPayload(
    nlohmann::json* payload,
    const std::vector<wf::proto::Data>& data) {
  for (const auto& key_value_pair : data) {
    std::vector<std::string> key_split = absl::StrSplit(key_value_pair.key(),
                                                        '.');

    nlohmann::json* part = payload;

    for (std::size_t i = 0; i < key_split.size() - 1; ++i) {
      const auto& key_part = key_split[i];

      if (!part->contains(key_part)) {
        (*part)[key_part] = nlohmann::json::object();
      } else if (!(*part)[key_part].is_object()) {
        return absl::DataLossError("data key part would overwrite "
                                   "non-container field");
      }

      *part = ((*part)[key_part]);
    }

    // part[key_split[-1]] is the location of the new json value.
    nlohmann::json json_value;
    absl::Status s = ExpandRenderValue(&json_value, key_value_pair.value());
    if (!s.ok()) {
      return s;
    }
    (*part)[key_split[key_split.size() - 1]] = json_value;
  }

  return absl::OkStatus();
}

absl::StatusOr<const inja::Template> Renderer::CacheHitOrParse(
    const std::string& key,
    std::istream* is) {
  std::ifstream ifs;
  if (template_cache_.contains(key)) {
    return template_cache_[key];
  }

  // Cache miss!
  if (is == nullptr) {
    ifs = std::ifstream(search_path_ / key);
    if (!ifs.is_open()) {
      return absl::NotFoundError(absl::StrFormat("no such template '%s'", key));
    }

    is = &ifs;
  }

  std::string src(std::istreambuf_iterator<char>(*is), {});

  try {
    inja::Template t = env_.parse(src);

    template_cache_[key] = t;
  } catch (const inja::InjaError& e) {
    return absl::AbortedError(std::string("failed to parse template: ") +
                              e.what());
  }

  return template_cache_[key];
}

}

