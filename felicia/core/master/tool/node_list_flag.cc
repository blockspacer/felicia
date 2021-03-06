// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/master/tool/node_list_flag.h"

#include "felicia/core/util/command_line_interface/text_style.h"

namespace felicia {

NodeListFlag::NodeListFlag() {
  {
    BoolFlag::Builder builder(MakeValueStore(&all_));
    auto flag = builder.SetShortName("-a")
                    .SetLongName("--all")
                    .SetHelp("List all the nodes")
                    .Build();
    all_flag_ = std::make_unique<BoolFlag>(flag);
  }
  {
    StringFlag::Builder builder(MakeValueStore(&publishing_topic_));
    auto flag = builder.SetShortName("-p")
                    .SetLongName("--publishing_topic")
                    .SetHelp("List nodes publishing a given topic")
                    .Build();
    publishing_topic_flag_ = std::make_unique<StringFlag>(flag);
  }
  {
    StringFlag::Builder builder(
        MakeValueStore<std::string>(&subscribing_topic_));
    auto flag = builder.SetShortName("-s")
                    .SetLongName("--subscribing_topic")
                    .SetHelp("List nodes subscribing a given topic")
                    .Build();
    subscribing_topic_flag_ = std::make_unique<StringFlag>(flag);
  }
  {
    StringFlag::Builder builder(MakeValueStore<std::string>(&name_));
    auto flag = builder.SetShortName("-n")
                    .SetLongName("--name")
                    .SetHelp("List a node whose name is equal to a given name")
                    .Build();
    name_flag_ = std::make_unique<StringFlag>(flag);
  }
}

NodeListFlag::~NodeListFlag() = default;

bool NodeListFlag::Parse(FlagParser& parser) {
  return PARSE_OPTIONAL_FLAG(parser, all_flag_, publishing_topic_flag_,
                             subscribing_topic_flag_, name_flag_);
}

bool NodeListFlag::Validate() const {
  return CheckIfOneOfFlagWasSet(all_flag_, publishing_topic_flag_,
                                subscribing_topic_flag_, name_flag_);
}

std::vector<std::string> NodeListFlag::CollectUsages() const {
  return {"[OPTIONS]"};
}

std::string NodeListFlag::Description() const { return "List nodes"; }

std::vector<NamedHelpType> NodeListFlag::CollectNamedHelps() const {
  return {
      std::make_pair(kYellowOptions,
                     std::vector<std::string>{
                         all_flag_->help(),
                         publishing_topic_flag_->help(),
                         subscribing_topic_flag_->help(),
                         name_flag_->help(),
                     }),
  };
}

}  // namespace felicia