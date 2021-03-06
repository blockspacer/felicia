// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_DRIVERS_CAMERA_CAMERA_DESCRIPTOR_H_
#define FELICIA_DRIVERS_CAMERA_CAMERA_DESCRIPTOR_H_

#include <string>
#include <vector>

#include "felicia/core/lib/base/export.h"

namespace felicia {
namespace drivers {

class FEL_EXPORT CameraDescriptor {
 public:
  CameraDescriptor();
  CameraDescriptor(const std::string& display_name,
                   const std::string& device_id, const std::string& model_id);
  CameraDescriptor(const CameraDescriptor& other);
  CameraDescriptor& operator=(const CameraDescriptor& other);
  ~CameraDescriptor();

  const std::string& display_name() const;
  const std::string& device_id() const;
  const std::string& model_id() const;

  std::string ToString() const;

 protected:
  std::string display_name_;
  std::string device_id_;
  std::string model_id_;
};

using CameraDescriptors = std::vector<CameraDescriptor>;

FEL_EXPORT std::ostream& operator<<(std::ostream& os,
                                    const CameraDescriptor& camera_descriptor);

FEL_EXPORT std::ostream& operator<<(
    std::ostream& os, const CameraDescriptors& camera_descriptors);

}  // namespace drivers
}  // namespace felicia

#endif  // FELICIA_DRIVERS_CAMERA_CAMERA_DESCRIPTOR_H_