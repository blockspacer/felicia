// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_LIB_UNIT_GEOMETRY_SIZE_H_
#define FELICIA_CORE_LIB_UNIT_GEOMETRY_SIZE_H_

#include "third_party/chromium/base/strings/string_number_conversions.h"
#include "third_party/chromium/base/strings/stringprintf.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/unit/unit_helper.h"
#include "felicia/core/protobuf/geometry.pb.h"

namespace felicia {

template <typename T>
class Size {
 public:
  constexpr Size() = default;
  constexpr Size(T width, T height)
      : width_(width > 0 ? width : 0), height_(height > 0 ? height : 0) {}
  constexpr Size(const Size& other) = default;
  Size& operator=(const Size& other) = default;

  constexpr T width() const { return width_; }
  constexpr T height() const { return height_; }

  void set_width(T width) { width_ = width; }
  void set_height(T height) { height_ = height; }

  constexpr T area() const { return CheckedArea().ValueOrDie(); }

  Size operator+(Size other) const {
    return {internal::SaturateAdd(width_, other.width_),
            internal::SaturateAdd(height_, other.height_)};
  }
  Size operator-(Size other) const {
    return {internal::SaturateSub(width_, other.width_),
            internal::SaturateSub(height_, other.height_)};
  }
  Size& operator+=(Size other) {
    width_ = internal::SaturateAdd(width_, other.width_);
    height_ = internal::SaturateAdd(height_, other.height_);
    return *this;
  }
  Size& operator-=(Size other) {
    width_ = internal::SaturateSub(width_, other.width_);
    height_ = internal::SaturateSub(height_, other.height_);
    return *this;
  }

  template <typename U>
  Size operator*(U a) const {
    return {internal::SaturateMul(width_, a),
            internal::SaturateMul(height_, a)};
  }
  template <typename U>
  Size operator/(U a) const {
    return {internal::SaturateDiv(width_, a),
            internal::SaturateDiv(height_, a)};
  }
  template <typename U>
  Size& operator*=(U a) {
    return *this = (*this * a);
  }
  template <typename U>
  Size& operator/=(U a) {
    return *this = (*this / a);
  }

  std::string ToString() const {
    return base::StringPrintf("(%s, %s)", base::NumberToString(width_).c_str(),
                              base::NumberToString(height_).c_str());
  }

 private:
  constexpr base::CheckedNumeric<T> CheckedArea() const {
    base::CheckedNumeric<T> checked_area = width();
    checked_area *= height();
    return checked_area;
  }

  T width_ = 0;
  T height_ = 0;
};

template <typename T>
inline bool operator==(const Size<T>& lhs, const Size<T>& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

template <typename T>
inline bool operator!=(const Size<T>& lhs, const Size<T>& rhs) {
  return !(lhs == rhs);
}

template <typename T>
inline Size<T> operator*(T a, Size<T> size) {
  return size * a;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Size<T>& size) {
  return os << size.ToString();
}

typedef Size<int> Sizei;
typedef Size<float> Sizef;
typedef Size<double> Sized;

template <typename MessageType, typename T>
MessageType SizeToSizeMessage(const Size<T>& size) {
  MessageType message;
  message.set_width(size.width());
  message.set_height(size.height());
  return message;
}

FEL_EXPORT SizeiMessage SizeiToSizeiMessage(const Sizei& size);
FEL_EXPORT SizefMessage SizefToSizefMessage(const Sizef& size);
FEL_EXPORT SizedMessage SizedToSizedMessage(const Sized& size);

template <typename T, typename MessageType>
Size<T> SizeMessageToSize(const MessageType& message) {
  return {message.width(), message.height()};
}

FEL_EXPORT Sizei SizeiMessageToSizei(const SizeiMessage& message);
FEL_EXPORT Sizef SizefMessageToSizef(const SizefMessage& message);
FEL_EXPORT Sized SizedMessageToSized(const SizedMessage& message);

}  // namespace felicia

#endif  // FELICIA_CORE_LIB_UNIT_GEOMETRY_SIZE_H_