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
    name = "application",
    srcs = ["application.cc"],
    hdrs = ["application.h"],
    deps = [
        ":middleware",
        ":router",
        "//webforge/core:renderer",
        "//webforge/http",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:string_view",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "middleware",
    srcs = ["middleware.cc"],
    hdrs = ["middleware.h"],
    deps = [
        "//webforge/http",
        "//webforge/http:date",
        "//webforge/http:strings",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:string_view",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "processor",
    srcs = ["processor.cc"],
    hdrs = ["processor.h"],
    deps = [
        ":middleware",
        "//webforge/core:data_cc_proto",
        "//webforge/http",
        "//webforge/http:date",
        "//webforge/http:strings",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "router",
    srcs = ["router.cc"],
    hdrs = ["router.h"],
    deps = [
        ":middleware",
        "//webforge/http",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
    visibility = ["//visibility:public"],
)

