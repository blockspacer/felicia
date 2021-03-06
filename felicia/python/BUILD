# Copyright (c) 2019 The Felicia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("//bazel:felicia_cc.bzl", "fel_cc_library")
load("//bazel:felicia_pybind.bzl", "fel_pybind_py_library")

package(default_visibility = ["//felicia:internal"])

fel_cc_library(
    name = "felicia_py_cc",
    deps = [
        "//felicia/core:felicia_init",
        "//felicia/python/channel:channel_py_cc",
        "//felicia/python/command_line_interface:command_line_interface_py_cc",
        "//felicia/python/communication:communication_py_cc",
        "//felicia/python/drivers:drivers_py_cc",
        "//felicia/python/master:master_proxy_py_cc",
        "//felicia/python/node:node_lifecycle_py_cc",
        "//felicia/python/thread:main_thread_py_cc",
    ],
)

py_library(
    name = "felicia_py_py",
    deps = [
        "//felicia/core/protobuf:protos_all_proto_py",
        "//felicia/drivers/camera:camera_proto_py",
        "//felicia/python/command_line_interface:command_line_interface_py_py",
        "//felicia/python/rpc:rpc_py_py",
    ],
)

py_library(
    name = "import_order_resolver",
    srcs = ["import_order_resolver.py"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

fel_pybind_py_library(
    name = "felicia_py",
    cc_deps = ["//felicia/python:felicia_py_cc"],
    cc_srcs = ["felicia_py.cc"],
    py_deps = ["//felicia/python:felicia_py_py"],
)
