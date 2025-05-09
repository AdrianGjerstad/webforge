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

proto_library(
    name = "data_proto",
    srcs = ["data.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "data_cc_proto",
    deps = [
        ":data_proto",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "minifier",
    srcs = ["minifier.cc"],
    hdrs = ["minifier.h"],
    deps = [
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/synchronization",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "minifier_test",
    srcs = ["minifier_test.cc"],
    deps = [
        ":minifier",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest",
    ],
    size = "small",
)

cc_library(
    name = "renderer",
    srcs = ["renderer.cc"],
    hdrs = ["renderer.h"],
    deps = [
        ":data_cc_proto",
        "//inja",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@nlohmann_json//:json",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "renderer_test",
    srcs = ["renderer_test.cc"],
    deps = [
        ":renderer",
        ":data_cc_proto",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest",
    ],
    size = "small",
)

