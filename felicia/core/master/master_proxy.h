// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FELICIA_CORE_MASTER_MASTER_PROXY_H_
#define FELICIA_CORE_MASTER_MASTER_PROXY_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "third_party/chromium/base/bind.h"
#include "third_party/chromium/base/callback.h"
#include "third_party/chromium/base/macros.h"
#include "third_party/chromium/base/no_destructor.h"
#include "third_party/chromium/base/synchronization/waitable_event.h"

#include "felicia/core/channel/channel.h"
#include "felicia/core/lib/base/export.h"
#include "felicia/core/master/heart_beat_signaller.h"
#include "felicia/core/master/master_client_interface.h"
#include "felicia/core/master/master_notification_watcher.h"
#include "felicia/core/node/node_lifecycle.h"

namespace felicia {

class PyMasterProxy;

class FEL_EXPORT MasterProxy final : public MasterClientInterface {
 public:
  static MasterProxy& GetInstance();

  const ClientInfo& client_info() const;

  void set_heart_beat_duration(base::TimeDelta heart_beat_duration);

#if defined(FEL_WIN_NODE_BINDING)
  Status StartMasterClient();

  bool is_client_info_set() const;
#endif  // defined(FEL_WIN_NODE_BINDING)

  // MasterClientInterface methods
  Status Start() override;
  Status Stop() override;

#define MASTER_METHOD(Method, method, cancelable)                         \
  void Method##Async(const Method##Request* request,                      \
                     Method##Response* response, StatusOnceCallback done) \
      override;
#include "felicia/core/master/rpc/master_method_list.h"
#undef MASTER_METHOD

  template <typename NodeTy, typename... Args,
            std::enable_if_t<std::is_base_of<NodeLifecycle, NodeTy>::value>* =
                nullptr>
  void RequestRegisterNode(const NodeInfo& node_info, Args&&... args);

 private:
  friend class base::NoDestructor<MasterProxy>;
  friend class PyMasterProxy;
  friend class TopicInfoWatcherNode;
  template <typename MessageTy>
  friend class Subscriber;
  template <typename ClientTy>
  friend class ServiceClient;

  MasterProxy();
  ~MasterProxy();

  void Setup(base::WaitableEvent* event);

  void OnHeartBeatSignallerStart(base::WaitableEvent* event,
                                 const ChannelSource& channel_source);

  void RegisterClient();

  void OnRegisterClient(base::WaitableEvent* event,
                        const RegisterClientRequest* request,
                        RegisterClientResponse* response, Status s);

  void OnRegisterNodeAsync(std::unique_ptr<NodeLifecycle> node,
                           const RegisterNodeRequest* request,
                           RegisterNodeResponse* response, Status s);

  void SubscribeTopicAsync(
      const SubscribeTopicRequest* request, SubscribeTopicResponse* response,
      StatusOnceCallback callback,
      MasterNotificationWatcher::NewTopicInfoCallback topic_info_callback);

  void RegisterServiceClientAsync(
      const RegisterServiceClientRequest* request,
      RegisterServiceClientResponse* response, StatusOnceCallback callback,
      MasterNotificationWatcher::NewServiceInfoCallback service_info_callback);

  std::unique_ptr<MasterClientInterface> master_client_interface_;

  ClientInfo client_info_;

  MasterNotificationWatcher master_notification_watcher_;
  HeartBeatSignaller heart_beat_signaller_;

  std::vector<std::unique_ptr<NodeLifecycle>> nodes_;

#if defined(FEL_WIN_NODE_BINDING)
  bool is_client_info_set_ = false;
#endif  // defined(FEL_WIN_NODE_BINDING)

  DISALLOW_COPY_AND_ASSIGN(MasterProxy);
};

template <typename NodeTy, typename... Args,
          std::enable_if_t<std::is_base_of<NodeLifecycle, NodeTy>::value>*>
void MasterProxy::RequestRegisterNode(const NodeInfo& node_info,
                                      Args&&... args) {
  std::unique_ptr<NodeLifecycle> node =
      std::make_unique<NodeTy>(std::forward<Args>(args)...);
  node->OnInit();

  RegisterNodeRequest* request = new RegisterNodeRequest();
  NodeInfo* new_node_info = request->mutable_node_info();
  new_node_info->CopyFrom(node_info);
  new_node_info->set_client_id(client_info_.id());
  RegisterNodeResponse* response = new RegisterNodeResponse();

  RegisterNodeAsync(
      request, response,
      base::BindOnce(&MasterProxy::OnRegisterNodeAsync, base::Unretained(this),
                     base::Passed(&node), base::Owned(request),
                     base::Owned(response)));
}

}  // namespace felicia

#endif  // FELICIA_CORE_MASTER_MASTER_PROXY_H_