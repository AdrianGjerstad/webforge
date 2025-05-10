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
// File: strings_test.cc
// -----------------------------------------------------------------------------
//
// This file defines tests for the HTTP Strings suite.
//

#include "webforge/http/strings.h"

#include "absl/container/flat_hash_map.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(HTTPStrings, CanURLEncodeUnsafeStrings) {
  // Base case (does it function?)
  EXPECT_EQ(wf::URLEncode("Hello, world!"), "Hello%2C+world%21");

  // Disable plus-spaces
  EXPECT_EQ(wf::URLEncode("Hello, world!", wf::UnsafeChars, false),
            "Hello%2C%20world%21");

  // Attacks: query string pollution attempts
  EXPECT_EQ(wf::URLEncode("foo&uid=123"), "foo%26uid%3D123");

  // Attacks: XSS attempts
  EXPECT_EQ(wf::URLEncode("<script>alert(1);</script>"),
            "%3Cscript%3Ealert%281%29%3B%3C%2Fscript%3E");
}

TEST(HTTPStrings, CanURLDecodeStrings) {
  // Base case (does it function?)
  EXPECT_EQ(wf::URLDecode("Hello%2C+world%21"), "Hello, world!");

  // Disable plus-spaces
  EXPECT_EQ(wf::URLDecode("Hello%2C+world%21", false), "Hello,+world!");
}

TEST(HTTPStrings, CanParseQueryStrings) {
  absl::flat_hash_map<std::string, std::string> data;
  // Reset test state for next test
  auto Reset = [&data]() {
    data.clear();
  };

  // Expect the output map to match *exactly* with a reference.
  auto ExpectMatch =
    [&data](const absl::flat_hash_map<std::string, std::string>& reference) {
    EXPECT_EQ(data.size(), reference.size());

    for (const auto& it : reference) {
      EXPECT_TRUE(data.contains(it.first));
      if (data.contains(it.first)) {
        EXPECT_EQ(data.at(it.first), it.second);
      }
    }
  };

  // Base case (does it function?)
  wf::ParseQueryString("foo=bar&baz=ham", &data);
  ExpectMatch({{"foo", "bar"}, {"baz", "ham"}});
  Reset();

  // Valueless data
  wf::ParseQueryString("auto", &data);
  ExpectMatch({{"auto", "1"}});
  Reset();

  wf::ParseQueryString("auto&q=data", &data);
  ExpectMatch({{"auto", "1"}, {"q", "data"}});
  Reset();

  wf::ParseQueryString("q=data&auto", &data);
  ExpectMatch({{"auto", "1"}, {"q", "data"}});
  Reset();

  wf::ParseQueryString("x=foo&auto&y=bar", &data);
  ExpectMatch({{"auto", "1"}, {"x", "foo"}, {"y", "bar"}});
  Reset();
}

TEST(HTTPStrings, CanRenderQueryStrings) {
  // Multiple possibilities, since the order doesn't matter and the underlying
  // implementation uses flat_hash_map which is unordered.
  EXPECT_THAT(wf::RenderQueryString({{"foo", "bar"}, {"baz", "ham"}}),
              testing::AnyOf(
                testing::Eq("foo=bar&baz=ham"),
                testing::Eq("baz=ham&foo=bar")));

  EXPECT_EQ(wf::RenderQueryString({{"q", "Hello, world!"}}),
            "q=Hello%2C+world%21");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

