#include "felicia/core/lib/unit/length.h"

namespace felicia {

Length::Length() = default;
Length::Length(int64_t length) : length_(length) {}

bool Length::operator==(Length other) const { return length_ == other.length_; }
bool Length::operator!=(Length other) const { return length_ != other.length_; }
bool Length::operator<(Length other) const { return length_ < other.length_; }
bool Length::operator<=(Length other) const { return length_ <= other.length_; }
bool Length::operator>(Length other) const { return length_ > other.length_; }
bool Length::operator>=(Length other) const { return length_ >= other.length_; }

Length Length::operator+(Length other) const {
  return SaturateAdd(length_, other.length_);
}
Length Length::operator-(Length other) const {
  return SaturateSub(length_, other.length_);
}
Length& Length::operator+=(Length other) { return *this = (*this + other); }
Length& Length::operator-=(Length other) { return *this = (*this - other); }

double Length::operator/(Length a) const { return length_ / a.length_; }

// static
Length Length::FromMillimeter(int64_t millimeter) { return Length(millimeter); }

// static
Length Length::FromCentimeter(int64_t centimeter) {
  return FromProduct(centimeter, kCentimeter);
}

// static
Length Length::FromCentimeterD(double centimeter) {
  return FromDouble(centimeter * kCentimeter);
}

// static
Length Length::FromMeter(int64_t meter) { return FromProduct(meter, kMeter); }

// static
Length Length::FromMeterD(double meter) { return FromDouble(meter * kMeter); }

// static
Length Length::FromKillometer(int64_t killometer) {
  return FromProduct(killometer, kKillometer);
}

// static
Length Length::FromKillometerD(double killometer) {
  return FromDouble(killometer * kKillometer);
}

// static
Length Length::FromFeet(int64_t feet) { return FromDouble(feet * kFeet); }

// static
Length Length::FromFeetD(double feet) { return FromDouble(feet * kFeet); }

// static
Length Length::FromInch(int64_t inch) { return FromDouble(inch * kInch); }

// static
Length Length::FromInchD(double inch) { return FromDouble(inch * kInch); }

// static
Length Length::Max() { return Length(std::numeric_limits<int64_t>::max()); }

// static
Length Length::Min() { return Length(std::numeric_limits<int64_t>::min()); }

int64_t Length::InMillimeter() const { return length_; }

double Length::InCentimeter() const { return length_ / kCentimeter; }

double Length::InMeter() const { return length_ / kMeter; }

double Length::InKillometer() const { return length_ / kKillometer; }

double Length::InFeet() const { return length_ / kInch; }

double Length::InInch() const { return length_ / kFeet; }

int64_t Length::length() const { return length_; }

// static
Length Length::SaturateAdd(int64_t value, int64_t value2) {
  ::base::CheckedNumeric<int64_t> rv(value);
  rv += value2;
  if (rv.IsValid()) return Length(rv.ValueOrDie());
  return Length::Max();
}

// static
Length Length::SaturateSub(int64_t value, int64_t value2) {
  ::base::CheckedNumeric<int64_t> rv(value);
  rv -= value2;
  if (rv.IsValid()) return Length(rv.ValueOrDie());
  return Length();
}

// static
Length Length::FromProduct(int64_t value, int64_t multiplier) {
  DCHECK(multiplier > 0);
  return value > std::numeric_limits<int64_t>::max() / multiplier
             ? Length::Max()
             : value < std::numeric_limits<int64_t>::min() / multiplier
                   ? Length::Min()
                   : Length(value * multiplier);
}

// static
Length Length::FromDouble(double value) {
  return Length(::base::saturated_cast<int64_t>(value));
}

std::ostream& operator<<(std::ostream& os, Length length) {
  os << ::base::NumberToString(length.length()) << " millimeter";
  return os;
}

}  // namespace felicia