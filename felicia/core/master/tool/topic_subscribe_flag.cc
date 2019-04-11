#include "felicia/core/master/tool/topic_subscribe_flag.h"

#include "felicia/core/util/command_line_interface/text_style.h"

namespace felicia {

TopicSubscribeFlag::TopicSubscribeFlag() {
  {
    BoolFlag::Builder builder(MakeValueStore(&all_));
    auto flag = builder.SetShortName("-a")
                    .SetLongName("--all")
                    .SetHelp("Subscribe all the topcis")
                    .Build();
    all_flag_ = std::make_unique<BoolFlag>(flag);
  }
  {
    StringFlag::Builder builder(MakeValueStore(&topic_));
    auto flag = builder.SetShortName("-t")
                    .SetLongName("--topic")
                    .SetHelp("Topic to subscribe")
                    .Build();
    topic_flag_ = std::make_unique<StringFlag>(flag);
  }
}

TopicSubscribeFlag::~TopicSubscribeFlag() = default;

bool TopicSubscribeFlag::Parse(FlagParser& parser) {
  return PARSE_OPTIONAL_FLAG(parser, all_flag_, topic_flag_);
}

bool TopicSubscribeFlag::Validate() const {
  return CheckIfOneOfFlagWasSet(all_flag_, topic_flag_);
}

std::vector<std::string> TopicSubscribeFlag::CollectUsages() const {
  return {"[OPTIONS]"};
}

std::string TopicSubscribeFlag::Description() const {
  return "Subscribe topics";
}

std::vector<NamedHelpType> TopicSubscribeFlag::CollectNamedHelps() const {
  return {
      std::make_pair(TextStyle::Yellow("Options:"),
                     std::vector<std::string>{
                         all_flag_->help(),
                         topic_flag_->help(),
                     }),
  };
}

}  // namespace felicia