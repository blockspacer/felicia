// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_MESSAGE_PROTOBUF_MESSAGE_IO_H_
#define FELICIA_CORE_MESSAGE_PROTOBUF_MESSAGE_IO_H_

#include "google/protobuf/message.h"

#include "third_party/chromium/base/strings/string_util.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/message/dynamic_protobuf_message.h"

namespace felicia {

namespace protobuf_internal {

FEL_EXPORT bool SerializeToString(const google::protobuf::Message* msg,
                                  std::string* text);

FEL_EXPORT bool ParseFromArray(google::protobuf::Message* msg,
                               const char* start, size_t size);

}  // namespace protobuf_internal

template <typename T>
class MessageIO<
    T, std::enable_if_t<std::is_base_of<google::protobuf::Message, T>::value>> {
 public:
  static MessageIOError Serialize(const T* protobuf_msg, std::string* text) {
    if (!protobuf_internal::SerializeToString(protobuf_msg, text))
      return MessageIOError::ERR_FAILED_TO_SERIALIZE;

    return MessageIOError::OK;
  }

  static MessageIOError Deserialize(const char* start, size_t size,
                                    T* protobuf_msg) {
    if (!protobuf_internal::ParseFromArray(protobuf_msg, start, size))
      return MessageIOError::ERR_FAILED_TO_PARSE;

    return MessageIOError::OK;
  }

  static std::string TypeName() { return T::descriptor()->full_name(); }

  static std::string Definition() { return T::descriptor()->DebugString(); }

  static std::string MD5Sum() { return base::EmptyString(); }
};

template <typename T>
class MessageIO<
    T, std::enable_if_t<std::is_same<DynamicProtobufMessage, T>::value>> {
 public:
  static MessageIOError Serialize(const T* protobuf_msg, std::string* text) {
    if (!protobuf_msg->SerializeToString(text))
      return MessageIOError::ERR_FAILED_TO_SERIALIZE;

    return MessageIOError::OK;
  }

  static MessageIOError Deserialize(const char* start, size_t size,
                                    T* protobuf_msg) {
    if (!protobuf_msg->ParseFromArray(start, size))
      return MessageIOError::ERR_FAILED_TO_PARSE;

    return MessageIOError::OK;
  }

  static std::string TypeName() { return base::EmptyString(); }

  static std::string Definition() { return base::EmptyString(); }

  static std::string MD5Sum() { return base::EmptyString(); }
};

}  // namespace felicia

#endif  // FELICIA_CORE_MESSAGE_PROTOBUF_MESSAGE_IO_H_