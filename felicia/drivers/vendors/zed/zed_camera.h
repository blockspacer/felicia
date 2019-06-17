#ifndef FELICIA_DRIVERS_VENDORS_ZED_CAMERA_H_
#define FELICIA_DRIVERS_VENDORS_ZED_CAMERA_H_

#include <sl/Camera.hpp>

#include "third_party/chromium/base/memory/weak_ptr.h"
#include "third_party/chromium/base/synchronization/waitable_event.h"
#include "third_party/chromium/base/threading/thread.h"

#include "felicia/drivers/camera/stereo_camera_interface.h"
#include "felicia/drivers/vendors/zed/zed_capability.h"

namespace felicia {

class ZedCamera : public StereoCameraInterface,
                  public ::base::SupportsWeakPtr<ZedCamera> {
 public:
  class ScopedCamera {
   public:
    ScopedCamera();
    ScopedCamera(ScopedCamera&& other);
    void operator=(ScopedCamera&& other);

    ~ScopedCamera();

    const ::sl::Camera* operator->() const { return get(); }
    ::sl::Camera* operator->() { return get(); }

    const ::sl::Camera* get() const { return camera_.get(); }
    ::sl::Camera* get() { return camera_.get(); }

   private:
    std::unique_ptr<::sl::Camera> camera_;

    DISALLOW_COPY_AND_ASSIGN(ScopedCamera);
  };

  ~ZedCamera();

  // StereoCameraInterface methods
  Status Init() override;
  Status Start(const CameraFormat& requested_camera_format,
               CameraFrameCallback left_camera_frame_callback,
               CameraFrameCallback right_camera_frame_callback,
               StatusCallback status_callback) override;
  Status Stop() override;

  Status SetCameraSettings(const CameraSettings& camera_settings) override;
  Status GetCameraSettingsInfo(
      CameraSettingsInfoMessage* camera_settings) override;

  ::sl::Camera* camera() { return camera_.get(); }

  const ::sl::Camera* camera() const { return camera_.get(); }

 private:
  friend class ZedCameraFactory;

  ZedCamera(const CameraDescriptor& camera_descriptor);

  void GetCameraSetting(::sl::CAMERA_SETTINGS camera_setting,
                        CameraSettingsModeValue* value);
  void GetCameraSetting(::sl::CAMERA_SETTINGS camera_setting,
                        CameraSettingsRangedValue* value);

  void DoGrab();
  void DoStop(::base::WaitableEvent* event, Status* s);

  static Status OpenCamera(const CameraDescriptor& camera_descriptor,
                           ::sl::InitParameters& params, ScopedCamera* camera);

  ScopedCamera camera_;

  ::base::Thread thread_;
  ::base::Lock lock_;
  bool is_stopping_ GUARDED_BY(lock_);

  DISALLOW_IMPLICIT_CONSTRUCTORS(ZedCamera);
};

}  // namespace felicia

#endif  // FELICIA_DRIVERS_VENDORS_ZED_CAMERA_H_