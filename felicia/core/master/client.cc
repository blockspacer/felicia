// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/master/client.h"

#include <algorithm>
#include <limits>

#include "third_party/chromium/base/memory/ptr_util.h"
#include "third_party/chromium/base/no_destructor.h"
#include "third_party/chromium/base/rand_util.h"

#include "felicia/core/util/uuid/generator.h"

namespace felicia {

namespace {

struct RandUint32Traits {
  static uint32_t Generate() {
    return base::RandGenerator(std::numeric_limits<uint32_t>::max());
  };

  static constexpr uint32_t InvalidValue() {
    return std::numeric_limits<uint32_t>::max();
  }
};

Generator<uint32_t, RandUint32Traits>& GetIDGenerator() {
  static base::NoDestructor<Generator<uint32_t, RandUint32Traits>> id_generator;
  return *id_generator;
}

}  // namespace

// static
std::unique_ptr<Client> Client::NewClient(const ClientInfo& client_info) {
  uint32_t id = GetIDGenerator().Generate();
  if (id == RandUint32Traits::InvalidValue()) {
    LOG(ERROR) << "Failed to generate id for client";
    return nullptr;
  }

  ClientInfo new_client_info;
  new_client_info.CopyFrom(client_info);
  new_client_info.set_id(id);
  return base::WrapUnique(new Client(new_client_info));
}

Client::Client(const ClientInfo& client_info) : client_info_(client_info) {}

Client::~Client() { GetIDGenerator().Return(client_info().id()); }

const ClientInfo& Client::client_info() const { return client_info_; }

void Client::AddNode(std::unique_ptr<Node> node) {
  DFAKE_SCOPED_LOCK(add_remove_);
  nodes_.push_back(std::move(node));
}

void Client::RemoveNode(const NodeInfo& node_info) {
  DFAKE_SCOPED_LOCK(add_remove_);
  nodes_.erase(
      std::remove_if(nodes_.begin(), nodes_.end(), NodeNameChecker{node_info}),
      nodes_.end());
}

bool Client::HasNode(const NodeInfo& node_info) const {
  DFAKE_SCOPED_LOCK(add_remove_);
  return std::find_if(nodes_.begin(), nodes_.end(),
                      NodeNameChecker{node_info}) != nodes_.end();
}

base::WeakPtr<Node> Client::FindNode(const NodeInfo& node_info) const {
  DFAKE_SCOPED_LOCK(add_remove_);
  auto it =
      std::find_if(nodes_.begin(), nodes_.end(), NodeNameChecker{node_info});
  if (it == nodes_.end()) {
    return nullptr;
  }

  return (*it)->AsWeakPtr();
}

std::vector<base::WeakPtr<Node>> Client::FindNodes(
    const NodeFilter& node_filter) const {
  DFAKE_SCOPED_LOCK(add_remove_);
  std::vector<base::WeakPtr<Node>> nodes;
  if (node_filter.all()) {
    for (auto& node : nodes_) {
      nodes.push_back(node->AsWeakPtr());
    }
  } else if (!node_filter.publishing_topic().empty()) {
    for (auto& node : nodes_) {
      if (node->IsPublishingTopic(node_filter.publishing_topic())) {
        nodes.push_back(node->AsWeakPtr());
        break;
      }
    }
  } else if (!node_filter.subscribing_topic().empty()) {
    for (auto& node : nodes_) {
      if (node->IsSubsribingTopic(node_filter.subscribing_topic())) {
        nodes.push_back(node->AsWeakPtr());
      }
    }
  } else if (!node_filter.requesting_service().empty()) {
    for (auto& node : nodes_) {
      if (node->IsRequestingService(node_filter.requesting_service())) {
        nodes.push_back(node->AsWeakPtr());
      }
    }
  } else if (!node_filter.serving_service().empty()) {
    for (auto& node : nodes_) {
      if (node->IsServingService(node_filter.serving_service())) {
        nodes.push_back(node->AsWeakPtr());
      }
    }
  } else if (!node_filter.name().empty()) {
    for (auto& node : nodes_) {
      if (node->name() == node_filter.name()) {
        nodes.push_back(node->AsWeakPtr());
      }
    }
  } else if (node_filter.watcher()) {
    for (auto& node : nodes_) {
      if (node->node_info().watcher()) {
        nodes.push_back(node->AsWeakPtr());
        break;
      }
    }
  }

  return nodes;
}

std::vector<TopicInfo> Client::FindTopicInfos(
    const TopicFilter& topic_filter) const {
  DFAKE_SCOPED_LOCK(add_remove_);
  std::vector<TopicInfo> topic_infos;
  if (topic_filter.all()) {
    for (auto& node : nodes_) {
      std::vector<TopicInfo> tmp_topic_infos = node->AllPublishingTopicInfos();
      topic_infos.insert(topic_infos.end(), tmp_topic_infos.begin(),
                         tmp_topic_infos.end());
    }
  } else if (!topic_filter.topic().empty()) {
    for (auto& node : nodes_) {
      if (node->IsPublishingTopic(topic_filter.topic())) {
        topic_infos.push_back(node->GetTopicInfo(topic_filter.topic()));
        break;
      }
    }
  }

  return topic_infos;
}

std::vector<ServiceInfo> Client::FindServiceInfos(
    const ServiceFilter& service_filter) const {
  DFAKE_SCOPED_LOCK(add_remove_);
  std::vector<ServiceInfo> service_infos;
  if (service_filter.all()) {
    for (auto& node : nodes_) {
      std::vector<ServiceInfo> tmp_service_infos =
          node->AllServingServiceInfos();
      service_infos.insert(service_infos.end(), tmp_service_infos.begin(),
                           tmp_service_infos.end());
    }
  } else if (!service_filter.service().empty()) {
    for (auto& node : nodes_) {
      if (node->IsServingService(service_filter.service())) {
        service_infos.push_back(node->GetServiceInfo(service_filter.service()));
        break;
      }
    }
  }

  return service_infos;
}

std::vector<std::string> Client::FindAllSubscribingTopics() const {
  DFAKE_SCOPED_LOCK(add_remove_);
  std::vector<std::string> topics;
  for (auto& node : nodes_) {
    std::vector<std::string> tmp_topics = node->AllSubscribingTopics();
    topics.insert(topics.end(), tmp_topics.begin(), tmp_topics.end());
  }

  return topics;
}

std::vector<std::string> Client::FindAllRequestingServices() const {
  DFAKE_SCOPED_LOCK(add_remove_);
  std::vector<std::string> services;
  for (auto& node : nodes_) {
    std::vector<std::string> tmp_services = node->AllRequestingServices();
    services.insert(services.end(), tmp_services.begin(), tmp_services.end());
  }

  return services;
}

}  // namespace felicia