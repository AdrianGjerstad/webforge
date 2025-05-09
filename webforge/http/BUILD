# Copyright (C) 2025 Adrian Gjerstad
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

cc_library(
    name = "cookie",
    srcs = ["cookie.cc"],
    hdrs = ["cookie.h"],
    deps = [
        ":strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "date",
    srcs = ["date.cc"],
    hdrs = ["date.h"],
    deps = [
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "http",
    srcs = ["http.cc"],
    hdrs = ["http.h"],
    deps = [
        ":cookie",
        ":strings",
        "//webforge/core:data_cc_proto",
        "//webforge/core:renderer",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
        "@nlohmann_json//:json",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "http_test",
    srcs = [
        "http_test.cc",
    ],
    deps = [
        ":http",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@nlohmann_json//:json",
    ],
    size = "small",
)

cc_library(
    name = "strings",
    srcs = [
        "mime.cc",
        "strings.cc",
    ],
    hdrs = ["strings.h"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "strings_test",
    srcs = [
        "strings_test.cc",
    ],
    deps = [
        ":strings",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@googletest//:gtest",
    ],
    size = "small",
)

