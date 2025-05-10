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
// File: renderer_test.cc
// -----------------------------------------------------------------------------
//
// This file contains a suite of unit tests for the wf::Renderer class, covering
// *all* of its functionality.
//

#include "webforge/core/renderer.h"

#include <sstream>
#include <string>

#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include <gtest/gtest.h>

#include "webforge/core/data.pb.h"

class RendererTest : public testing::Test {
protected:
  wf::Renderer renderer_;
};

TEST_F(RendererTest, CanRenderTextAndCache) {
  std::istringstream src("Hello, {{ name }}!");
  std::ostringstream output;
  wf::proto::Data name;
  name.set_key("name");
  name.mutable_value()->set_text("world");

  absl::Status render_status = renderer_.Render("foo", &src, {name}, &output);
  ASSERT_THAT(render_status, absl_testing::IsOk());

  EXPECT_EQ(output.str(), "Hello, world!");

  // Okay cool, we can render. Can we cache?
  src.str("");
  src.clear();
  output.str("");
  output.clear();
  render_status = renderer_.Render("foo", &src, {name}, &output);
  ASSERT_THAT(render_status, absl_testing::IsOk());

  EXPECT_EQ(output.str(), "Hello, world!");

  // When we tell it to flush the cache, does it?
  renderer_.FlushCache();
  src.clear();
  src.str("No cache");
  output.clear();
  output.str("");
  render_status = renderer_.Render("foo", &src, {name}, &output);
  ASSERT_THAT(render_status, absl_testing::IsOk());

  EXPECT_EQ(output.str(), "No cache");
}

TEST_F(RendererTest, CanRenderHTMLSafely) {
  std::string src_str("{{ content }}");
  std::istringstream src(src_str);
  std::ostringstream output;

  wf::proto::Data content;
  content.set_key("content");
  content.mutable_value()->set_text("<script>alert(1);</script>");

  absl::Status render_status = renderer_.RenderHTML("foo", &src, {content},
                                                    &output);
  ASSERT_THAT(render_status, absl_testing::IsOk());

  EXPECT_EQ(output.str(), "&lt;script&gt;alert(1);&lt;/script&gt;");
}

TEST_F(RendererTest, CanRenderNestedObjects) {
  std::string src_str("{{ data.text }} "
                      "{{ data.integer }} "
                      "{{ data.real }} "
                      "{% for d in data.vector %}"
                      "{{ d }} "
                      "{% endfor %}");
  std::istringstream src(src_str);
  std::ostringstream output;

  wf::proto::Data text;
  wf::proto::Data integer;
  wf::proto::Data real;
  wf::proto::Data vector;

  // Populate data fields
  text.set_key("data.text");
  text.mutable_value()->set_text("Text");
  integer.set_key("data.integer");
  integer.mutable_value()->set_integer(123);
  real.set_key("data.real");
  real.mutable_value()->set_real(3.14);
  
  vector.set_key("data.vector");
  wf::proto::VectorValue* vector_value =
    vector.mutable_value()->mutable_vector();
  vector_value->add_vector()->set_integer(1);
  vector_value->add_vector()->set_integer(2);
  vector_value->add_vector()->set_integer(3);

  // Render
  absl::Status render_status = renderer_.Render("foo", &src, {text, integer,
                                                              real, vector},
                                                &output);
  ASSERT_THAT(render_status, absl_testing::IsOk());

  EXPECT_EQ(output.str(), "Text 123 3.14 1 2 3 ");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

