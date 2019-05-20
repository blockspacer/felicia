#ifndef FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_DEPTH_CAMERA_CC_REALSENSE_RS_CAMERA_FLAG_H_
#define FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_DEPTH_CAMERA_CC_REALSENSE_RS_CAMERA_FLAG_H_

#include "felicia/examples/learn/message_communication/depth_camera/cc/depth_camera_flag.h"

namespace felicia {

class RsCameraFlag : public DepthCameraFlag {
 public:
  RsCameraFlag();
  ~RsCameraFlag();

  const StringDefaultFlag* imu_topic_flag() const {
    return imu_topic_flag_.get();
  }

  bool Parse(FlagParser& parser) override;

  AUTO_DEFINE_USAGE_AND_HELP_TEXT_METHODS(name_flag_, device_list_flag_,
                                          device_index_flag_, color_topic_flag_,
                                          depth_topic_flag_, imu_topic_flag_,
                                          synched_flag_)

 private:
  std::string imu_topic_;
  std::unique_ptr<StringDefaultFlag> imu_topic_flag_;
};

}  // namespace felicia

#endif  // FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_DEPTH_CAMERA_CC_REALSENSE_RS_CAMERA_FLAG_H_