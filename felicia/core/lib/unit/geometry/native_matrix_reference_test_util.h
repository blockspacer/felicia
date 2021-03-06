// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/lib/unit/geometry/native_matrix_reference.h"

namespace felicia {

template <typename MatrixType, typename MatrixType2>
void ExpectEqualMatrix(const MatrixType& lhs, const MatrixType2& rhs) {
  typedef ConstNativeMatrixRef<MatrixType> ConstMatrixTypeRef;
  typedef ConstNativeMatrixRef<MatrixType2> ConstMatrixTypeRef2;
  ConstMatrixTypeRef lhs_ref(lhs);
  ConstMatrixTypeRef2 rhs_ref(rhs);
  EXPECT_EQ(lhs_ref.rows(), rhs_ref.rows());
  EXPECT_EQ(lhs_ref.cols(), rhs_ref.cols());
  for (int i = 0; i < lhs_ref.rows(); ++i) {
    for (int j = 0; j < lhs_ref.cols(); ++j) {
      EXPECT_NEAR(lhs_ref.at(i, j), rhs_ref.at(i, j), 1e-5);
    }
  }
}

template <typename T, typename MatrixType>
void InitMatrix(T from, T step, T* to, MatrixType* matrix) {
  typedef NativeMatrixRef<MatrixType> MatrixTypeRef;
  MatrixTypeRef matrix_ref(*matrix);
  T data = from;
  for (int i = 0; i < matrix_ref.rows(); ++i) {
    for (int j = 0; j < matrix_ref.cols(); ++j) {
      matrix_ref.at(i, j) = data;
      data += step;
    }
  }
  *to = data;
}

}  // namespace felicia