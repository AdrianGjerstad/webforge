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
// File: minifier_test.cc
// -----------------------------------------------------------------------------
//
// This file tests the interface of wf::Minifier. The goal of each test SHOULD,
// best-case scenario, be to verify that the output from obviously-not-minified
// code should be shorter and functionally the same. In practice, this likely
// means we are going to have to assume what the output SHOULD be, and test for
// equality.
//

#include "webforge/core/minifier.h"

#include <sstream>

#include <iostream>

#include "absl/status/status.h"
#include <gtest/gtest.h>

class MinifierTest : public testing::Test {
protected:
  void PrepareStringStreams(const std::string& input,
                            std::istringstream* is,
                            std::ostringstream* os) {
    is->clear();
    is->str(input);
    os->clear();
    os->str("");
  }

  wf::Minifier minifier_;
};

TEST_F(MinifierTest, CanMinifyHtml) {
  std::istringstream is;
  std::ostringstream os;

  PrepareStringStreams(
    "<!doctype html>\n"
    "<html>\n"
    "  <head>\n"
    "    <title>Hello World</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Hello World</h1>\n"
    "    <p>Hello World</p>\n"
    "  </body>\n"
    "</html>\n", &is, &os);

  absl::Status s = minifier_.Minify(wf::SourceType::kHtml, &is, &os);
  ASSERT_TRUE(s.ok());

  EXPECT_EQ(os.str(), "<!doctype html>"
                      "<html><head><title>Hello World</title></head>"
                      "<body><h1>Hello World</h1><p>Hello World</p></body>"
                      "</html>");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

