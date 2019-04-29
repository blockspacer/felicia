#ifndef FELICIA_CORE_MASTER_TOPIC_INFO_WATCHER_H_
#define FELICIA_CORE_MASTER_TOPIC_INFO_WATCHER_H_

#include <memory>

#include "third_party/chromium/base/callback.h"
#include "third_party/chromium/base/containers/flat_map.h"
#include "third_party/chromium/base/macros.h"

#include "felicia/core/channel/channel.h"
#include "felicia/core/lib/base/task_runner_interface.h"
#include "felicia/core/protobuf/master_data.pb.h"

namespace felicia {

extern Bytes kTopicInfoBytes;

class TopicInfoWatcher {
 public:
  using NewTopicInfoCallback =
      ::base::RepeatingCallback<void(const TopicInfo&)>;

  TopicInfoWatcher();

  const ChannelSource& channel_source() const { return channel_source_; }

  void RegisterCallback(const std::string& topic,
                        NewTopicInfoCallback callback);

  void RegisterAllTopicCallback(NewTopicInfoCallback callback);

  void UnregisterCallback(const std::string& topic);

  void Start();

 private:
  void DoAccept();
  void OnAccept(const Status& s);

  void WatchNewTopicInfo();
  void OnNewTopicInfo(const Status& s);

  ChannelSource channel_source_;
  TopicInfo topic_info_;
  std::unique_ptr<Channel<TopicInfo>> channel_;
  ::base::flat_map<std::string, NewTopicInfoCallback> callback_map_;
  NewTopicInfoCallback all_topic_callback_;

  DISALLOW_COPY_AND_ASSIGN(TopicInfoWatcher);
};

}  // namespace felicia

#endif  // FELICIA_CORE_MASTER_TOPIC_INFO_WATCHER_H_