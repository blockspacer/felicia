# Copyright (c) 2019 The Felicia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("//bazel:felicia_proto.bzl", "fel_proto_library")

package(default_visibility = ["//felicia:internal"])

fel_proto_library(
    name = "protos_all_proto",
    srcs = [
        "bounding_box.proto",
        "channel.proto",
        "data.proto",
        "error_codes.proto",
        "geometry.proto",
        "human.proto",
        "master.proto",
        "master_data.proto",
        "ui.proto",
    ],
    default_header = True,
    export_proto = True,
    visibility = ["//visibility:public"],
)

fel_proto_library(
    name = "protos_all",
    srcs = [],
    default_header = True,
    export_proto = True,
    protodeps = [
        ":protos_all_proto",
        "//felicia/drivers/camera:camera_proto",
        "//felicia/drivers/imu:imu_proto",
        "//felicia/drivers/lidar:lidar_proto",
        "//felicia/map:map_proto",
        "//felicia/slam:camera_proto",
    ],
    visibility = ["//visibility:public"],
)
