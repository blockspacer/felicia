#include "felicia/python/master/master_proxy_py.h"

#include "third_party/chromium/base/memory/ptr_util.h"
#include "third_party/chromium/base/strings/stringprintf.h"
#include "third_party/chromium/build/build_config.h"

#include "felicia/core/master/master_proxy.h"
#include "felicia/python/type_conversion/callback.h"
#include "felicia/python/type_conversion/protobuf.h"

namespace felicia {

// static
Status PyMasterProxy::Start() { return MasterProxy::GetInstance().Start(); }

// static
Status PyMasterProxy::Stop() { return MasterProxy::GetInstance().Stop(); }

// static
void PyMasterProxy::Run() { MasterProxy::GetInstance().Run(); }

// static
void PyMasterProxy::RequestRegisterNode(py::function constructor,
                                        const NodeInfo& node_info,
                                        py::args args, py::kwargs kwargs) {
  MasterProxy& master_proxy = MasterProxy::GetInstance();

  RegisterNodeRequest* request = new RegisterNodeRequest();
  NodeInfo* new_node_info = request->mutable_node_info();
  new_node_info->set_client_id(master_proxy.client_info_.id());
  new_node_info->set_name(node_info.name());
  RegisterNodeResponse* response = new RegisterNodeResponse();

  py::object object = constructor(*args, **kwargs);
  object.inc_ref();
  NodeLifecycle* node = object.cast<NodeLifecycle*>();

  py::gil_scoped_release release;
  node->OnInit();
  master_proxy.RegisterNodeAsync(
      request, response,
      base::BindOnce(&PyMasterProxy::OnRegisterNodeAsync, object, request,
                     response));
}

// static
void PyMasterProxy::OnRegisterNodeAsync(py::object object,
                                        const RegisterNodeRequest* request,
                                        RegisterNodeResponse* response,
                                        Status s) {
  MasterProxy& master_proxy = MasterProxy::GetInstance();
  if (!master_proxy.IsBoundToCurrentThread()) {
    master_proxy.PostTask(
        FROM_HERE, base::BindOnce(&PyMasterProxy::OnRegisterNodeAsync, object,
                                  base::Owned(request), base::Owned(response),
                                  std::move(s)));
    return;
  }

  if (!s.ok()) {
    Status new_status(s.error_code(),
                      base::StringPrintf("Failed to register node : %s",
                                         s.error_message().c_str()));
    NodeLifecycle* node = object.cast<NodeLifecycle*>();
    node->OnError(new_status);
    return;
  }

  std::unique_ptr<NodeInfo> node_info(response->release_node_info());
  NodeLifecycle* node = object.cast<NodeLifecycle*>();
  node->OnDidCreate(std::move(*node_info));
}

void AddMasterProxy(py::module& m) {
  py::class_<PyMasterProxy>(m, "MasterProxy")
      .def_static("start", &PyMasterProxy::Start,
                  py::call_guard<py::gil_scoped_release>())
      .def_static("stop", &PyMasterProxy::Stop,
                  py::call_guard<py::gil_scoped_release>())
      .def_static("run", &PyMasterProxy::Run,
                  py::call_guard<py::gil_scoped_release>())
      .def_static("request_register_node", &PyMasterProxy::RequestRegisterNode,
                  py::arg("constructor"), py::arg("node_info"))
      .def_static("post_task",
                  [](py::function callback) {
                    MasterProxy& master_proxy = MasterProxy::GetInstance();
                    master_proxy.PostTask(
                        FROM_HERE,
                        base::BindOnce(&PyClosure::Invoke,
                                       base::Owned(new PyClosure(callback))));
                  },
                  py::arg("callback"), py::call_guard<py::gil_scoped_release>())
      .def_static("post_delayed_task",
                  [](py::function callback, base::TimeDelta delay) {
                    MasterProxy& master_proxy = MasterProxy::GetInstance();
                    master_proxy.PostDelayedTask(
                        FROM_HERE,
                        base::BindOnce(&PyClosure::Invoke,
                                       base::Owned(new PyClosure(callback))),
                        delay);
                  },
                  py::arg("callback"), py::arg("delay"),
                  py::call_guard<py::gil_scoped_release>());
}

}  // namespace felicia