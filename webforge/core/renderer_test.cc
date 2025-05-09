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
  ASSERT_TRUE(render_status.ok());

  EXPECT_EQ(output.str(), "Hello, world!");

  // Okay cool, we can render. Can we cache?
  src.str("");
  src.clear();
  output.str("");
  output.clear();
  render_status = renderer_.Render("foo", &src, {name}, &output);
  ASSERT_TRUE(render_status.ok());

  EXPECT_EQ(output.str(), "Hello, world!");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

