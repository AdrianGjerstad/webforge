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
# -----------------------------------------------------------------------------
#
# This file is an addition to the open-sourced and MIT-licensed pantor/inja. It
# is not originally from that repository, but rather created for WebForge for
# the purpose of use with Bazel.
#

cc_library(
    name = "inja",
    hdrs = glob(["include/inja/*.hpp"]),
    deps = [
        "@nlohmann_json//:json",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

