// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/node/topic_info_watcher_node.h"

#include "third_party/chromium/base/bind.h"

#include "felicia/core/master/master_proxy.h"

namespace felicia {

TopicInfoWatcherNode::TopicInfoWatcherNode(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {}

TopicInfoWatcherNode::~TopicInfoWatcherNode() = default;

void TopicInfoWatcherNode::OnInit() {
  MasterProxy& master_proxy = MasterProxy::GetInstance();
  master_proxy.master_notification_watcher_.RegisterAllTopicInfoCallback(
      base::BindRepeating(&TopicInfoWatcherNode::Delegate::OnNewTopicInfo,
                          base::Unretained(delegate_.get())));
}

void TopicInfoWatcherNode::OnDidCreate(NodeInfo node_info) {
  DCHECK(node_info.watcher());
  node_info_ = std::move(node_info);
}

void TopicInfoWatcherNode::OnError(Status s) {
  LOG(ERROR) << s;
  MasterProxy& master_proxy = MasterProxy::GetInstance();
  master_proxy.master_notification_watcher_.UnregisterAllTopicInfoCallback();
  delegate_->OnError(std::move(s));
}

}  // namespace felicia