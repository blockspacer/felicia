// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TEXT_STYLE_H_
#define FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TEXT_STYLE_H_

#include "third_party/chromium/base/strings/strcat.h"
#include "third_party/chromium/base/strings/string_piece.h"

#include "felicia/core/lib/base/export.h"

namespace felicia {

class FEL_EXPORT TextStyle {
 public:
#define STYLE_METHOD(Style) static std::string Style(base::StringPiece text);

  STYLE_METHOD(Red)
  STYLE_METHOD(Green)
  STYLE_METHOD(Blue)
  STYLE_METHOD(Yellow)
  STYLE_METHOD(Bold)

#undef STYLE_METHOD

 private:
  static std::string ApplyStyle(const char* style, base::StringPiece text);

  static constexpr const char* kRed = "\033[91m";
  static constexpr const char* kGreen = "\033[32m";
  static constexpr const char* kBlue = "\033[34m";
  static constexpr const char* kYellow = "\033[93m";
  static constexpr const char* kBold = "\033[1m";
  static constexpr const char* kNone = "\033[0m";
};

}  // namespace felicia

#endif  // FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TEXT_STYLE_H_
