// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(FEL_PY_BINDING)

#include "felicia/python/communication/serialized_message_subscriber_py.h"

#include "felicia/python/message/message_util.h"
#include "felicia/python/type_conversion/callback.h"
#include "felicia/python/type_conversion/protobuf.h"

namespace felicia {

namespace {

class PyMessageCallback {
 public:
  PyMessageCallback(py::object message_type, TopicInfo::ImplType impl_type,
                    py::function callback)
      : message_type_(message_type),
        impl_type_(impl_type),
        callback_(callback) {}

  void Invoke(SerializedMessage&& message) {
    std::string text = std::move(message).serialized();

    py::gil_scoped_acquire acquire;
    py::object py_message = message_type_();
    Status s = Deserialize(text, impl_type_, &py_message);
    if (!s.ok()) {
      return;
    }

    callback_(std::move(py_message));
  }

 private:
  py::object message_type_;
  TopicInfo::ImplType impl_type_;
  py::function callback_;
};

}  // namespace

PySerializedMessageSubscriber::PySerializedMessageSubscriber(
    py::object message_type, TopicInfo::ImplType impl_type)
    : message_type_(message_type) {
#if defined(HAS_ROS)
  message_md5_sum_ = GetMessageMD5SumFromPyObject(message_type, impl_type);
#endif  // defined(HAS_ROS)
  message_type_name_ = GetMessageTypeNameFromPyObject(message_type, impl_type);
  impl_type_ = impl_type;
}

void PySerializedMessageSubscriber::RequestSubscribe(
    const NodeInfo& node_info, const std::string& topic, int channel_types,
    const communication::Settings& settings,
    py::function py_on_message_callback,
    py::function py_on_message_error_callback, py::function py_callback) {
  OnMessageCallback on_message_callback;
  StatusCallback on_message_error_callback;
  StatusOnceCallback callback;

  if (!py_on_message_callback.is_none()) {
    on_message_callback = base::BindRepeating(
        &PyMessageCallback::Invoke,
        base::Owned(new PyMessageCallback(message_type_, impl_type_,
                                          py_on_message_callback)));
  }

  if (!py_on_message_error_callback.is_none()) {
    on_message_error_callback = base::BindRepeating(
        &PyStatusCallback::Invoke,
        base::Owned(new PyStatusCallback(py_on_message_error_callback)));
  }

  if (!py_callback.is_none()) {
    callback = base::BindOnce(&PyStatusCallback::Invoke,
                              base::Owned(new PyStatusCallback(py_callback)));
  }

  py::gil_scoped_release release;
  SerializedMessageSubscriber::RequestSubscribe(
      node_info, topic, channel_types, settings, on_message_callback,
      on_message_error_callback, std::move(callback));
}

void PySerializedMessageSubscriber::RequestUnsubscribe(
    const NodeInfo& node_info, const std::string& topic,
    py::function py_callback) {
  StatusOnceCallback callback;
  if (!py_callback.is_none()) {
    callback = base::BindOnce(&PyStatusCallback::Invoke,
                              base::Owned(new PyStatusCallback(py_callback)));
  }

  py::gil_scoped_release release;
  SerializedMessageSubscriber::RequestUnsubscribe(node_info, topic,
                                                  std::move(callback));
}

void AddSerializedMessageSubscriber(py::module& m) {
  py::class_<PySerializedMessageSubscriber>(m, "Subscriber")
      .def(py::init<py::object, TopicInfo::ImplType>(), py::arg("message_type"),
           py::arg("impl_type") = TopicInfo::PROTOBUF)
      .def("is_registering", &PySerializedMessageSubscriber::IsRegistering)
      .def("is_registered", &PySerializedMessageSubscriber::IsRegistered)
      .def("is_unregistering", &PySerializedMessageSubscriber::IsUnregistering)
      .def("is_unregistered", &PySerializedMessageSubscriber::IsUnregistered)
      .def("is_started", &PySerializedMessageSubscriber::IsStarted)
      .def("is_stopped", &PySerializedMessageSubscriber::IsStopped)
      .def("request_subscribe",
           [](PySerializedMessageSubscriber& self, const NodeInfo& node_info,
              const std::string& topic, int channel_types,
              const communication::Settings& settings,
              py::function on_message_callback) {
             self.RequestSubscribe(node_info, topic, channel_types, settings,
                                   on_message_callback, py::none(), py::none());
           },
           py::arg("node_info"), py::arg("topic"), py::arg("channel_types"),
           py::arg("settings"), py::arg("on_message_callback"))
      .def("request_subscribe",
           [](PySerializedMessageSubscriber& self, const NodeInfo& node_info,
              const std::string& topic, int channel_types,
              const communication::Settings& settings,
              py::function on_message_callback,
              py::function on_message_error_callback) {
             self.RequestSubscribe(node_info, topic, channel_types, settings,
                                   on_message_callback,
                                   on_message_error_callback, py::none());
           },
           py::arg("node_info"), py::arg("topic"), py::arg("channel_types"),
           py::arg("settings"), py::arg("on_message_callback"),
           py::arg("on_message_error_callback").none(true))
      .def("request_subscribe",
           [](PySerializedMessageSubscriber& self, const NodeInfo& node_info,
              const std::string& topic, int channel_types,
              const communication::Settings& settings,
              py::function on_message_callback,
              py::function on_message_error_callback, py::function callback) {
             self.RequestSubscribe(node_info, topic, channel_types, settings,
                                   on_message_callback,
                                   on_message_error_callback, callback);
           },
           py::arg("node_info"), py::arg("topic"), py::arg("channel_types"),
           py::arg("settings"), py::arg("on_message_callback"),
           py::arg("on_message_error_callback").none(true),
           py::arg("callback").none(true))
      .def("request_unsubscribe",
           [](PySerializedMessageSubscriber& self, const NodeInfo& node_info,
              const std::string& topic) {
             self.RequestUnsubscribe(node_info, topic, py::none());
           },
           py::arg("node_info"), py::arg("topic"))
      .def("request_unsubscribe",
           [](PySerializedMessageSubscriber& self, const NodeInfo& node_info,
              const std::string& topic, py::function callback) {
             self.RequestUnsubscribe(node_info, topic, callback);
           },
           py::arg("node_info"), py::arg("topic"),
           py::arg("callback").none(true));
}

}  // namespace felicia

#endif  // defined(FEL_PY_BINDING)