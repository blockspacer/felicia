// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_COMMUNICATION_SERVICE_CLIENT_H_
#define FELICIA_CORE_COMMUNICATION_SERVICE_CLIENT_H_

#include <memory>
#include <string>

#include "third_party/chromium/base/bind.h"
#include "third_party/chromium/base/callback.h"
#include "third_party/chromium/base/compiler_specific.h"
#include "third_party/chromium/base/macros.h"

#include "felicia/core/communication/register_state.h"
#include "felicia/core/master/master_proxy.h"
#include "felicia/core/thread/main_thread.h"

namespace felicia {

template <typename ClientTy>
class ServiceClient {
 public:
  using OnConnectCallback = base::RepeatingCallback<void(ServiceInfo::Status)>;

  ServiceClient() = default;

  ALWAYS_INLINE bool IsRegistering() const {
    return register_state_.IsRegistering();
  }
  ALWAYS_INLINE bool IsRegistered() const {
    return register_state_.IsRegistered();
  }
  ALWAYS_INLINE bool IsUnregistering() const {
    return register_state_.IsUnregistering();
  }
  ALWAYS_INLINE bool IsUnregistered() const {
    return register_state_.IsUnregistered();
  }

  ClientTy* operator->() { return &client_; }

  void RequestRegister(const NodeInfo& node_info, const std::string& service,
                       OnConnectCallback on_connect_callback,
                       StatusOnceCallback callback = StatusOnceCallback());

  void RequestUnregister(const NodeInfo& node_info, const std::string& service,
                         StatusOnceCallback callback = StatusOnceCallback());

 protected:
  void OnRegisterServiceClientAsync(OnConnectCallback on_connect_callback,
                                    StatusOnceCallback callback, Status s);

  void OnUnregisterServiceClientAsync(StatusOnceCallback callback, Status s);

  void OnFindServiceServer(const ServiceInfo& service_info);

  void OnConnect(Status s);

  ClientTy client_;
  OnConnectCallback on_connect_callback_;

  communication::RegisterState register_state_;
};

template <typename ClientTy>
void ServiceClient<ClientTy>::RequestRegister(
    const NodeInfo& node_info, const std::string& service,
    OnConnectCallback on_connect_callback, StatusOnceCallback callback) {
  MainThread& main_thread = MainThread::GetInstance();
  if (!main_thread.IsBoundToCurrentThread()) {
    main_thread.PostTask(
        FROM_HERE, base::BindOnce(&ServiceClient<ClientTy>::RequestRegister,
                                  base::Unretained(this), node_info, service,
                                  on_connect_callback, std::move(callback)));
    return;
  }

  if (!IsUnregistered()) {
    internal::LogOrCallback(std::move(callback),
                            register_state_.InvalidStateError());
    return;
  }

  register_state_.ToRegistering(FROM_HERE);

  RegisterServiceClientRequest request;
  *request.mutable_node_info() = node_info;
  request.set_service(service);
  RegisterServiceClientResponse response;

  MasterProxy& master_proxy = MasterProxy::GetInstance();
  master_proxy.RegisterServiceClientAsync(
      &request, &response,
      base::BindOnce(&ServiceClient<ClientTy>::OnRegisterServiceClientAsync,
                     base::Unretained(this), on_connect_callback,
                     std::move(callback)),
      base::BindRepeating(&ServiceClient<ClientTy>::OnFindServiceServer,
                          base::Unretained(this)));
}

template <typename ClientTy>
void ServiceClient<ClientTy>::RequestUnregister(const NodeInfo& node_info,
                                                const std::string& service,
                                                StatusOnceCallback callback) {
  MainThread& main_thread = MainThread::GetInstance();
  if (!main_thread.IsBoundToCurrentThread()) {
    main_thread.PostTask(
        FROM_HERE, base::BindOnce(&ServiceClient<ClientTy>::RequestUnregister,
                                  base::Unretained(this), node_info, service,
                                  std::move(callback)));
    return;
  }

  if (!IsRegistered()) {
    internal::LogOrCallback(std::move(callback),
                            register_state_.InvalidStateError());
    return;
  }

  register_state_.ToUnregistering(FROM_HERE);

  UnregisterServiceClientRequest request;
  *request.mutable_node_info() = node_info;
  request.set_service(service);
  UnregisterServiceClientResponse response;

  MasterProxy& master_proxy = MasterProxy::GetInstance();
  master_proxy.UnregisterServiceClientAsync(
      &request, &response,
      base::BindOnce(&ServiceClient<ClientTy>::OnUnregisterServiceClientAsync,
                     base::Unretained(this), std::move(callback)));
}

template <typename ClientTy>
void ServiceClient<ClientTy>::OnRegisterServiceClientAsync(
    OnConnectCallback on_connect_callback, StatusOnceCallback callback,
    Status s) {
  if (!IsRegistering()) {
    internal::LogOrCallback(std::move(callback),
                            register_state_.InvalidStateError());
    return;
  }

  if (!s.ok()) {
    register_state_.ToUnregistered(FROM_HERE);
    internal::LogOrCallback(std::move(callback), std::move(s));
    return;
  }

  on_connect_callback_ = on_connect_callback;

  register_state_.ToRegistered(FROM_HERE);
  internal::LogOrCallback(std::move(callback), std::move(s));
}

template <typename ClientTy>
void ServiceClient<ClientTy>::OnUnregisterServiceClientAsync(
    StatusOnceCallback callback, Status s) {
  if (!IsUnregistering()) {
    internal::LogOrCallback(std::move(callback),
                            register_state_.InvalidStateError());
    return;
  }

  if (!s.ok()) {
    register_state_.ToRegistered(FROM_HERE);
    internal::LogOrCallback(std::move(callback), std::move(s));
    return;
  }

  register_state_.ToUnregistered(FROM_HERE);
  internal::LogOrCallback(std::move(callback), std::move(s));
}

template <typename ClientTy>
void ServiceClient<ClientTy>::OnFindServiceServer(
    const ServiceInfo& service_info) {
  MainThread& main_thread = MainThread::GetInstance();
  DCHECK(main_thread.IsBoundToCurrentThread());
  if (IsRegistering() || IsUnregistering()) {
    main_thread.PostTask(
        FROM_HERE, base::BindOnce(&ServiceClient<ClientTy>::OnFindServiceServer,
                                  base::Unretained(this), service_info));
    return;
  }

  if (IsUnregistered()) return;

  DCHECK(IsRegistered()) << register_state_.ToString();

  if (service_info.status() == ServiceInfo::UNREGISTERED) {
    client_.Shutdown();
    on_connect_callback_.Run(ServiceInfo::UNREGISTERED);
    return;
  }

  const IPEndPoint& ip_endpoint =
      service_info.service_source().channel_defs(0).ip_endpoint();
  client_.set_service_info(service_info);
  client_.Connect(ip_endpoint,
                  base::BindOnce(&ServiceClient<ClientTy>::OnConnect,
                                 base::Unretained(this)));
}

template <typename ClientTy>
void ServiceClient<ClientTy>::OnConnect(Status s) {
  if (s.ok()) {
    if (client_.Run().ok()) on_connect_callback_.Run(ServiceInfo::REGISTERED);
  }
}

}  // namespace felicia

#endif  // FELICIA_CORE_COMMUNICATION_SERVICE_CLIENT_H_