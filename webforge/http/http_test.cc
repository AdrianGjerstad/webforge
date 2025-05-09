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
// File: http_test.cc
// -----------------------------------------------------------------------------
//
// This file defines tests for the HTTP suite.
//

#include "webforge/http/http.h"

#include <memory>
#include <sstream>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

class RequestTest : public testing::Test {
protected:
  void SetUp() override {
    // Set some defaults for tests that doesn't explicitly test abnormal cases.
    req_.UsingTLS(true);
    req_.Method("GET");
    req_.Path("/");
    req_.Version("HTTP/3.0");
    req_.Header("Host", "localhost");
  }

  wf::Request req_;
};

class ResponseTest : public testing::Test {
protected:
  void SetUp() override {
    res_.Version("HTTP/3.0");
    res_.Status(200);
  }

  wf::Response res_;
};

TEST_F(RequestTest, CanParseURLEncodedBody) {
  auto stream = std::make_shared<std::istringstream>(
    "csrf=deadbeef&fname=Adrian&lname=Gjerstad&message=Hello%2C+world%21"
  );
  req_.Method("POST");
  req_.Header("Content-Type", "application/x-www-form-urlencoded");
  req_.Header("Content-Length", std::to_string(stream->str().size()));
  req_.Stream(stream);

  absl::flat_hash_map<std::string, std::string> data;
  absl::Status s = req_.ParseURLEncoded(&data);
  ASSERT_THAT(s, absl_testing::IsOk());

  // There are 4 fields in the above "body"
  EXPECT_EQ(data.size(), 4);

  // Check their values
  ASSERT_TRUE(data.contains("csrf"));
  ASSERT_TRUE(data.contains("fname"));
  ASSERT_TRUE(data.contains("lname"));
  ASSERT_TRUE(data.contains("message"));

  EXPECT_EQ(data.at("csrf"), "deadbeef");
  EXPECT_EQ(data.at("fname"), "Adrian");
  EXPECT_EQ(data.at("lname"), "Gjerstad");
  // URLDecode()'d message
  EXPECT_EQ(data.at("message"), "Hello, world!");
}

TEST_F(RequestTest, ParseURLEncodedChecksPreconditions) {
  auto stream = std::make_shared<std::istringstream>("foo=bar");
  req_.Method("POST");
  absl::flat_hash_map<std::string, std::string> data;
  absl::Status s = req_.ParseURLEncoded(&data);

  // Neither Content-Type nor Content-Length exists.
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));
  
  // Reset
  stream->str("foo=bar");
  req_.Header("Content-Length", std::to_string(stream->str().size()));

  s = req_.ParseURLEncoded(&data);

  // Content-Type does not exist.
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));

  // Reset
  req_.Method("GET");
  req_.Header("Content-Type", "application/x-www-form-urlencoded");
  stream->str("foo=bar");

  s = req_.ParseURLEncoded(&data);
  
  // Content-Type and Content-Length exist and are correct. Method GET does not
  // allow request bodies.
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));

  // Reset
  req_.Method("POST");
  req_.Header("Content-Type", "text/plain");
  stream->str("foo=bar");

  s = req_.ParseURLEncoded(&data);
  // Test that it checks the value of Content-Type
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST_F(RequestTest, CanParseJSONBody) {
  std::string test_data(
    "{"
      "\"csrf\": \"deadbeef\","
      "\"fname\": \"Adrian\","
      "\"lname\": \"Gjerstad\","
      "\"message\": \"Hello, world!\""
    "}"
  );

  auto stream = std::make_shared<std::istringstream>(test_data);
  req_.Method("POST");
  req_.Header("Content-Type", "application/json");
  req_.Stream(stream);

  nlohmann::json data;
  absl::Status s = req_.ParseJSON(&data);
  ASSERT_THAT(s, absl_testing::IsOk());

  // There are 4 fields in the above "body"
  EXPECT_EQ(data.size(), 4);

  // Check their values
  ASSERT_TRUE(data.contains("csrf"));
  ASSERT_TRUE(data.contains("fname"));
  ASSERT_TRUE(data.contains("lname"));
  ASSERT_TRUE(data.contains("message"));

  EXPECT_EQ(data.at("csrf"), "deadbeef");
  EXPECT_EQ(data.at("fname"), "Adrian");
  EXPECT_EQ(data.at("lname"), "Gjerstad");
  EXPECT_EQ(data.at("message"), "Hello, world!");
}

TEST_F(RequestTest, ParseJSONChecksPreconditions) {
  std::string test_data(
    "{"
      "\"csrf\": \"deadbeef\","
      "\"fname\": \"Adrian\","
      "\"lname\": \"Gjerstad\","
      "\"message\": \"Hello, world!\""
    "}"
  );

  auto stream = std::make_shared<std::istringstream>(test_data);
  req_.Method("POST");
  req_.Stream(stream);
  nlohmann::json data;
  absl::Status s = req_.ParseJSON(&data);

  // Content-Type does not exist.
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));

  // Reset
  req_.Method("GET");
  req_.Header("Content-Type", "application/json");
  stream->str(test_data);

  s = req_.ParseJSON(&data);
  
  // Content-Type exists and is correct. Method GET does not allow request
  // bodies.
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));

  // Reset
  req_.Method("POST");
  req_.Header("Content-Type", "text/plain");
  stream->str(test_data);

  s = req_.ParseJSON(&data);

  // Test that it checks the value of Content-Type
  EXPECT_THAT(s, absl_testing::StatusIs(absl::StatusCode::kFailedPrecondition));

  // Reset
  req_.Header("Content-Type", "application/json");
  stream->str(test_data);

  s = req_.ParseJSON(&data);

  // Observe that the presence of a Content-Length does not matter
  EXPECT_THAT(s, absl_testing::IsOk());
}

// For the next tests...
namespace {

class WriteHeadWriter : public wf::ResponseWriter {
public:
  WriteHeadWriter() : head_written_(false) {}

  absl::Status WriteHead(const wf::Response& res) override {
    head_written_ = true;
    return absl::OkStatus();
  }

  absl::Status WriteChunk(absl::string_view chunk) override {
    return absl::OkStatus();
  }

  void End() override {}

  bool HeadWritten() const {
    return head_written_;
  }

private:
  bool head_written_;
};

class WriteEndGuaranteeWriter : public wf::ResponseWriter {
public:
  WriteEndGuaranteeWriter() : has_ended_(false) {}

  absl::Status WriteHead(const wf::Response& res) override {
    return absl::OkStatus();
  }

  absl::Status WriteChunk(absl::string_view chunk) override {
    return absl::InternalError("example error");
  }

  void End() override {
    has_ended_ = true;
  }

  bool HasEnded() const {
    return has_ended_;
  }

private:
  bool has_ended_;
};

}

TEST_F(ResponseTest, WriteDoesWriteHead) {
  auto writer = std::make_shared<WriteHeadWriter>();
  res_.UseWriter(writer);

  absl::Status s = res_.Write("Hello, world!");
  ASSERT_THAT(s, absl_testing::IsOk());

  EXPECT_TRUE(writer->HeadWritten());
}

TEST_F(ResponseTest, EndDoesWriteHead) {
  auto writer = std::make_shared<WriteHeadWriter>();
  res_.UseWriter(writer);

  absl::Status s = res_.End("Hello, world!");
  ASSERT_THAT(s, absl_testing::IsOk());

  EXPECT_TRUE(writer->HeadWritten());
}

TEST_F(ResponseTest, EndGuaranteedToDoWriterEnd) {
  auto writer = std::make_shared<WriteEndGuaranteeWriter>();
  res_.UseWriter(writer);

  absl::Status s = res_.End("Hello, world!");
  EXPECT_THAT(s, testing::Not(absl_testing::IsOk()));

  EXPECT_TRUE(writer->HasEnded());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

