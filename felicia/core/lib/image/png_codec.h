// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Followings are taken and modified from
// https://github.com/chromium/chromium/blob/5db095c2653f332334d56ad739ae5fe1053308b1/ui/gfx/codec/png_codec.h

#ifndef FELICIA_CORE_LIB_IMAGE_PNG_CODEC_H_
#define FELICIA_CORE_LIB_IMAGE_PNG_CODEC_H_

#include <stddef.h>

#include <vector>

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/error/status.h"
#include "felicia/core/lib/image/image.h"

namespace felicia {

class FEL_EXPORT PngCodec {
 public:
  struct Options {
    // compression_level should be between 0 and 9. The higher, the more
    // compressed.
    int compression_level = 6;
  };

  static Status Encode(const Image& image, const Options& options,
                       std::vector<unsigned char>* output);

  static Status Decode(const unsigned char* input, size_t input_size,
                       Image* image);
};

}  // namespace felicia

#endif  // FELICIA_CORE_LIB_IMAGE_PNG_CODEC_H_