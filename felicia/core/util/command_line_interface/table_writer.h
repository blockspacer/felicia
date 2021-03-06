// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TABLE_WRITER_H_
#define FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TABLE_WRITER_H_

#include <string>
#include <vector>

#include "third_party/chromium/base/macros.h"
#include "third_party/chromium/base/strings/string_piece.h"

#include "felicia/core/lib/base/export.h"

namespace felicia {

class FEL_EXPORT TableWriter {
 public:
  TableWriter(const TableWriter& other);
  ~TableWriter();

  struct FEL_EXPORT Column {
    Column(std::string title, int width);
    Column(const Column& other);
    ~Column();

    std::string title;
    size_t width = 16;
  };

  void SetElement(size_t row, size_t col, base::StringPiece element);
  std::string ToString() const;

 private:
  friend class TableWriterBuilder;
  TableWriter();

  std::vector<Column> heads_;
  std::vector<std::vector<std::string>> elements_;
};

class FEL_EXPORT TableWriterBuilder {
 public:
  TableWriterBuilder();
  ~TableWriterBuilder();

  TableWriterBuilder& AddColumn(const TableWriter::Column& column);

  TableWriter Build() const;

 private:
  TableWriter writer_;

  DISALLOW_COPY_AND_ASSIGN(TableWriterBuilder);
};

}  // namespace felicia

#endif  // FELICIA_CORE_UTIL_COMMAND_LINE_INTERFACE_TABLE_WRITER_H_