# Copyright (c) 2019 The Felicia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("//bazel:felicia_cc.bzl", "fel_cc_library")

package(default_visibility = ["//felicia:internal"])

fel_cc_library(
    name = "communication_py_cc",
    srcs = [
        "communication_py.cc",
        "ros_serialized_service_client_py.cc",
        "ros_serialized_service_server_py.cc",
        "serialized_message_publisher_py.cc",
        "serialized_message_subscriber_py.cc",
        "service_client_py.cc",
        "service_server_py.cc",
    ],
    hdrs = [
        "communication_py.h",
        "ros_serialized_service_client_py.h",
        "ros_serialized_service_server_py.h",
        "serialized_message_publisher_py.h",
        "serialized_message_subscriber_py.h",
        "service_client_py.h",
        "service_server_py.h",
    ],
    deps = [
        "//felicia/core/communication",
        "//felicia/python/message",
        "//felicia/python/rpc:rpc_py_cc",
        "//felicia/python/type_conversion",
    ],
)
