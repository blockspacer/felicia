#include "felicia/core/master/tool/topic_flag.h"

#include "third_party/chromium/base/logging.h"
#include "third_party/chromium/base/strings/string_util.h"
#include "third_party/chromium/base/strings/stringprintf.h"

#include "felicia/core/lib/strings/str_util.h"
#include "felicia/core/util/command_line_interface/text_style.h"

namespace felicia {

static const char* kLs = "ls";

TopicFlag::TopicFlag() : current_command_(COMMAND_SELF) {
  {
    StringChoicesFlag::Builder builder(MakeValueStore<std::string>(
        &command_, ::base::EmptyString(), Choices<std::string>{kLs}));
    auto flag = builder.SetName("COMMAND").Build();
    command_flag_ = std::make_unique<StringChoicesFlag>(flag);
  }
}

TopicFlag::~TopicFlag() = default;

bool TopicFlag::Parse(FlagParser& parser) {
  switch (current_command_) {
    case COMMAND_SELF:
      if (command_flag_->Parse(parser)) {
        if (strings::Equals(command_, kLs)) {
          current_command_ = COMMAND_LIST;
        }
        parser.set_program_name(::base::StringPrintf(
            "%s %s", parser.program_name().c_str(), command_.c_str()));
        return true;
      }
      return false;
    case COMMAND_LIST:
      return list_delegate_.Parse(parser);
  }
}

bool TopicFlag::Validate() const {
  switch (current_command_) {
    case COMMAND_SELF:
      return false;
    case COMMAND_LIST:
      return list_delegate_.Validate();
  }
}

std::vector<std::string> TopicFlag::CollectUsages() const {
  switch (current_command_) {
    case COMMAND_SELF:
      return {"COMMAND"};
    case COMMAND_LIST:
      return list_delegate_.CollectUsages();
  }
}

std::string TopicFlag::Description() const {
  switch (current_command_) {
    case COMMAND_SELF:
      return "Manage topics";
    case COMMAND_LIST:
      return list_delegate_.Description();
  }
}

std::vector<NamedHelpType> TopicFlag::CollectNamedHelps() const {
  switch (current_command_) {
    case COMMAND_SELF: {
      return {
          std::make_pair(TextStyle::Yellow("Commands:"),
                         std::vector<std::string>{
                             MakeNamedHelpText(kLs, "List topics"),
                         }),
      };
    }
    case COMMAND_LIST:
      return list_delegate_.CollectNamedHelps();
  }
}

}  // namespace felicia