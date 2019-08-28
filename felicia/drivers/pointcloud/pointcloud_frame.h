#ifndef FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_
#define FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_

#include "third_party/chromium/base/callback.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/containers/string_vector.h"
#include "felicia/core/lib/error/status.h"
#include "felicia/core/lib/unit/geometry/point.h"
#include "felicia/core/lib/unit/ui/color.h"
#include "felicia/drivers/pointcloud/pointcloud_frame_message.pb.h"

namespace felicia {
namespace drivers {

class EXPORT PointcloudFrame {
 public:
  PointcloudFrame();
  PointcloudFrame(const std::string& points, const std::string& colors,
                  base::TimeDelta timestamp);
  PointcloudFrame(std::string&& points, std::string&& colors,
                  base::TimeDelta timestamp) noexcept;
  PointcloudFrame(const PointcloudFrame& other);
  PointcloudFrame(PointcloudFrame&& other) noexcept;

  PointcloudFrame& operator=(const PointcloudFrame& other);
  PointcloudFrame& operator=(PointcloudFrame&& other);

  const StringVector& points() const;
  StringVector& points();
  const StringVector& colors() const;
  StringVector& colors();

  void set_timestamp(base::TimeDelta time);
  base::TimeDelta timestamp() const;

  PointcloudFrameMessage ToPointcloudFrameMessage(bool copy = true);
  Status FromPointcloudFrameMessage(const PointcloudFrameMessage& message);
  Status FromPointcloudFrameMessage(PointcloudFrameMessage&& message);

 private:
  StringVector points_;
  StringVector colors_;
  base::TimeDelta timestamp_;
};

typedef base::RepeatingCallback<void(PointcloudFrame&&)>
    PointcloudFrameCallback;

}  // namespace drivers
}  // namespace felicia

#endif  // FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_