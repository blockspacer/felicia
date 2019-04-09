#include "felicia/drivers/camera/camera_factory.h"

#include "third_party/chromium/base/memory/ptr_util.h"
#include "third_party/chromium/build/build_config.h"

#if defined(OS_LINUX)
#include "felicia/drivers/camera/linux/v4l2_camera.h"
using Camera = felicia::V4l2Camera;
#else

namespace felicia {

class FakeCamera : public CameraInterface {
 public:
  FakeCamera(const CameraDescriptor& descriptor) {}

  Status Init() override { return Status::OK(); }
  Status Start(CameraFrameCallback callback) override { return Status::OK(); }
  Status Close() override { return Status::OK(); }

  StatusOr<CameraFormat> GetFormat() override { return CameraFormat(); }
  Status SetFormat(CameraFormat format) override { return Status::OK(); }
};

using Camera = FakeCamera;

}  // namespace felicia
#endif

namespace felicia {

std::unique_ptr<CameraInterface> CameraFactory::NewCamera(
    const CameraDescriptor& descriptor) {
  return ::base::WrapUnique(new Camera(descriptor));
}

}  // namespace felicia