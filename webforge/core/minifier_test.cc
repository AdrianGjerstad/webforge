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

#include <iostream>
#include <sstream>
#include <string>

#include "absl/status/status.h"
#include <gtest/gtest.h>

// Definition moved here to avoid spamming fork() and slowing tests down
wf::Minifier minifier;

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

  absl::Status s = minifier.Minify(wf::SourceType::kHtml, &is, &os);
  ASSERT_TRUE(s.ok());

  EXPECT_EQ(os.str(), "<!doctype html>"
                      "<html><head><title>Hello World</title></head>"
                      "<body><h1>Hello World</h1><p>Hello World</p></body>"
                      "</html>");
}

TEST_F(MinifierTest, CanMinifyCss) {
  std::istringstream is;
  std::ostringstream os;

  PrepareStringStreams(
    ".class {\n"
    "  margin: 0px;\n"
    "  padding: 0px;\n"
    "}\n", &is, &os);

  absl::Status s = minifier.Minify(wf::SourceType::kCss, &is, &os);
  ASSERT_TRUE(s.ok());

  EXPECT_EQ(os.str(), ".class{margin:0;padding:0}");
}

TEST_F(MinifierTest, CanMinifyJavaScript) {
  std::istringstream is;
  std::ostringstream os;

  PrepareStringStreams(
    "(function(longName) {\n"
    "  alert('Hello ' + longName);\n"
    "})('Adrian');\n", &is, &os);

  absl::Status s = minifier.Minify(wf::SourceType::kJavaScript, &is, &os);
  ASSERT_TRUE(s.ok());

  // So apparently html-minifier is incredible because it also does static code
  // analysis. The below code is *not* what I thought it would produce, but I am
  // pleasantly surprised.
  EXPECT_EQ(os.str(), "alert(\"Hello Adrian\")");
}

TEST_F(MinifierTest, CanMinifyXml) {
  std::istringstream is;
  std::ostringstream os;

  PrepareStringStreams(
    "<?xml version=\"1.0\" charset=\"UTF-8\"?>\n"
    "<urlset>\n"
    "  <url>\n"
    "    <loc>https://example.com/foobar/</loc>\n"
    "    <lastmod>2025-04-26</lastmod>\n"
    "    <priority>1.0</priority>\n"
    "  </url>\n"
    "</urlset>\n", &is, &os);

  absl::Status s = minifier.Minify(wf::SourceType::kXml, &is, &os);
  ASSERT_TRUE(s.ok());

  // There is a space after the opening XML tag. I have no idea what set of
  // options are required to get rid of it.
  EXPECT_EQ(os.str(),
    "<?xml version=\"1.0\" charset=\"UTF-8\"?> "
    "<urlset><url><loc>https://example.com/foobar/</loc>"
    "<lastmod>2025-04-26</lastmod><priority>1.0</priority>"
    "</url></urlset>");
}

// No need for fuzz tests (in theory) because html-minifier has its own test
// suite.

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

