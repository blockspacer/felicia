// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_PYTHON_COMMUNICATION_SERIALIZED_MESSAGE_SUBSCRIBER_PY_H_
#define FELICIA_PYTHON_COMMUNICATION_SERIALIZED_MESSAGE_SUBSCRIBER_PY_H_

#if defined(FEL_PY_BINDING)

#include "pybind11/pybind11.h"

#include "felicia/core/communication/serialized_message_subscriber.h"

namespace py = pybind11;

namespace felicia {

class PySerializedMessageSubscriber : public SerializedMessageSubscriber {
 public:
  explicit PySerializedMessageSubscriber(
      py::object message_type,
      TopicInfo::ImplType impl_type = TopicInfo::PROTOBUF);

  void RequestSubscribe(const NodeInfo& node_info, const std::string& topic,
                        int channel_types,
                        const communication::Settings& settings,
                        py::function py_on_message_callback,
                        py::function py_on_message_error_callback = py::none(),
                        py::function py_callback = py::none());

  void RequestUnsubscribe(const NodeInfo& node_info, const std::string& topic,
                          py::function py_callback = py::none());

 private:
  py::object message_type_;
};

void AddSerializedMessageSubscriber(py::module& m);

}  // namespace felicia

#endif  // defined(FEL_PY_BINDING)

#endif  // FELICIA_PYTHON_COMMUNICATION_SERIALIZED_MESSAGE_SUBSCRIBER_PY_H_