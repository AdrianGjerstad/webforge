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
    name = "flags",
    hdrs = ["flags.h"],
    deps = [
        "@abseil-cpp//absl/flags:flag",
    ],
    visibility = ["//webforge:__subpackages__"],
)

cc_binary(
    name = "webforge",
    srcs = ["main.cc"],
    deps = [
        ":flags",
        "@abseil-cpp//absl/flags:config",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/flags:usage",
        "@abseil-cpp//absl/log:flags",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/log:log",
    ],
)

