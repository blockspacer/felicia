#ifndef FELICIA_PYTHON_RPC_SERVER_PY_H_
#define FELICIA_PYTHON_RPC_SERVER_PY_H_

#include "pybind11/pybind11.h"

#include "felicia/core/rpc/server_interface.h"

namespace py = pybind11;

namespace felicia {
namespace rpc {

class PyServer : public ServerInterface {
 public:
  using ServerInterface::ServerInterface;

  Status Start() override {
    PYBIND11_OVERLOAD_PURE(
        Status,          /* Return type */
        ServerInterface, /* Parent class */
        Start,           /* Name of function in C++ (must match Python name) */
    );
  }

  Status Run() override {
    PYBIND11_OVERLOAD_PURE(
        Status,          /* Return type */
        ServerInterface, /* Parent class */
        Run,             /* Name of function in C++ (must match Python name) */
    );
  }

  Status Shutdown() override {
    PYBIND11_OVERLOAD_PURE(
        Status,          /* Return type */
        ServerInterface, /* Parent class */
        Shutdown,        /* Name of function in C++ (must match Python name) */
    );
  }

  std::string GetServiceTypeName() const override {
    PYBIND11_OVERLOAD_PURE(std::string,        /* Return type */
                           ServerInterface,    /* Parent class */
                           GetServiceTypeName, /* Name of function in C++ (must
                                                  match Python name) */
    );
  }
};

class PyServerBridge {
 public:
  PyServerBridge();
  explicit PyServerBridge(py::object server);

  ChannelDef channel_def() const;

  void set_service_info(const ServiceInfo& service_info);

  Status Start();

  Status Run();

  Status Shutdown();

  std::string GetServiceTypeName() const;

 private:
  py::object server_;
};

void AddServer(py::module& m);

}  // namespace rpc
}  // namespace felicia

#endif  // FELICIA_PYTHON_RPC_SERVER_PY_H_