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
    name = "cgi",
    srcs = ["cgi.cc"],
    hdrs = ["cgi.h"],
    deps = [
        "//webforge/http",
        "//webforge/http:strings",
        "//webforge/site:application",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:string_view",
    ],
    visibility = ["//visibility:public"],
)

