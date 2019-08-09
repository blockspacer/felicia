#include "felicia/drivers/camera/camera_frame.h"

#include "libyuv.h"
#include "third_party/chromium/base/logging.h"

#include "felicia/core/lib/memory/memory_util.h"
#include "felicia/drivers/camera/camera_frame_util.h"

namespace felicia {

CameraFrame::CameraFrame() = default;

CameraFrame::CameraFrame(std::unique_ptr<uint8_t> data, size_t length,
                         const CameraFormat& camera_format,
                         base::TimeDelta timestamp)
    : data_(std::move(data)),
      length_(length),
      camera_format_(camera_format),
      timestamp_(timestamp) {}

CameraFrame::CameraFrame(CameraFrame&& other) noexcept
    : data_(std::move(other.data_)),
      length_(other.length_),
      camera_format_(other.camera_format_),
      timestamp_(other.timestamp_) {}

CameraFrame& CameraFrame::operator=(CameraFrame&& other) = default;

CameraFrame::~CameraFrame() = default;

std::unique_ptr<uint8_t> CameraFrame::data() { return std::move(data_); }

const uint8_t* CameraFrame::data_ptr() const { return data_.get(); }

size_t CameraFrame::length() const { return length_; }

const CameraFormat& CameraFrame::camera_format() const {
  return camera_format_;
}

int CameraFrame::width() const { return camera_format_.width(); }

int CameraFrame::height() const { return camera_format_.height(); }

float CameraFrame::frame_rate() const { return camera_format_.frame_rate(); }

PixelFormat CameraFrame::pixel_format() const {
  return camera_format_.pixel_format();
}

void CameraFrame::set_timestamp(base::TimeDelta timestamp) {
  timestamp_ = timestamp;
}

base::TimeDelta CameraFrame::timestamp() const { return timestamp_; }

CameraFrameMessage CameraFrame::ToCameraFrameMessage() const {
  CameraFrameMessage message;

  message.set_data(data_ptr(), length_);
  *message.mutable_camera_format() = camera_format_.ToCameraFormatMessage();
  message.set_timestamp(timestamp_.InMicroseconds());

  return message;
}

// static
CameraFrame CameraFrame::FromCameraFrameMessage(
    const CameraFrameMessage& message) {
  const std::string& data = message.data();
  std::unique_ptr<uint8_t> data_ptr(new uint8_t[data.length()]);
  memcpy(data_ptr.get(), data.c_str(), data.length());

  return {std::move(data_ptr), data.length(),
          CameraFormat::FromCameraFormatMessage(message.camera_format()),
          base::TimeDelta::FromMicroseconds(message.timestamp())};
}

// static
CameraFrame CameraFrame::FromCameraFrameMessage(CameraFrameMessage&& message) {
  std::string* data = message.release_data();
  size_t length = data->length();
  std::unique_ptr<uint8_t> data_ptr = StdStringToUniquePtr<uint8_t>(*data);

  return {std::move(data_ptr), length,
          CameraFormat::FromCameraFormatMessage(message.camera_format()),
          base::TimeDelta::FromMicroseconds(message.timestamp())};
}

#if defined(HAS_OPENCV)
namespace {

int ToCvType(const CameraFrame* camera_frame) {
  if (!camera_frame->camera_format().HasFixedSizedChannelPixelFormat())
    return -1;
  int channel =
      camera_frame->length() / (camera_frame->width() * camera_frame->height());
  return CV_MAKETYPE(CV_8U, channel);
}

}  // namespace

bool CameraFrame::ToCvMat(cv::Mat* out) {
  int type = ToCvType(this);
  if (type == -1) return false;

  uint8_t* data = data_.release();
  *out = cv::Mat{camera_format_.height(), camera_format_.width(), type, data};
  return true;
}

bool CameraFrame::CloneToCvMat(cv::Mat* out) const {
  int type = ToCvType(this);
  if (type == -1) return false;

  *out = cv::Mat{camera_format_.height(), camera_format_.width(), type};
  memcpy(out->data, data_.get(), length_);
  return true;
}

// static
CameraFrame CameraFrame::FromCvMat(cv::Mat mat,
                                   const CameraFormat& camera_format,
                                   base::TimeDelta timestamp) {
  size_t length = mat.rows * mat.cols * mat.dims;
  std::unique_ptr<uint8_t> data_ptr(new uint8_t[length]);
  memcpy(data_ptr.get(), mat.data, length);

  return {std::move(data_ptr), length, camera_format, timestamp};
}
#endif

namespace {

base::Optional<CameraFrame> ConvertToBGRA(const uint8_t* data,
                                          size_t data_length,
                                          const CameraFormat& camera_format,
                                          base::TimeDelta timestamp) {
  PixelFormat pixel_format = camera_format.pixel_format();
  libyuv::FourCC src_format;

  if (pixel_format == PIXEL_FORMAT_BGRA) {
    LOG(ERROR) << "Its format is already PIXEL_FORMAT_BGRA.";
  }

  src_format = camera_format.ToLibyuvPixelFormat();
  if (src_format == libyuv::FOURCC_ANY) return base::nullopt;

  const int width = camera_format.width();
  const int height = camera_format.height();
  CameraFormat bgra_camera_format(width, height, PIXEL_FORMAT_BGRA,
                                  camera_format.frame_rate());
  size_t length = bgra_camera_format.AllocationSize();
  std::unique_ptr<uint8_t> tmp_bgra(new uint8_t[length]);
  if (libyuv::ConvertToARGB(data, data_length, tmp_bgra.get(), width * 4,
                            0 /* crop_x_pos */, 0 /* crop_y_pos */, width,
                            height, width, height,
                            libyuv::RotationMode::kRotate0, src_format) != 0) {
    return base::nullopt;
  }

  return CameraFrame(std::move(tmp_bgra), length, bgra_camera_format,
                     timestamp);
}

}  // namespace

base::Optional<CameraFrame> ConvertToRequestedPixelFormat(
    const uint8_t* data, size_t data_length, const CameraFormat& camera_format,
    PixelFormat requested_pixel_format, base::TimeDelta timestamp) {
  if (requested_pixel_format == PIXEL_FORMAT_MJPEG) {
    return base::nullopt;
  } else if (requested_pixel_format == PIXEL_FORMAT_BGRA) {
    return ConvertToBGRA(data, data_length, camera_format, timestamp);
  } else {
    auto bgra_camera_frame_opt =
        ConvertToBGRA(data, data_length, camera_format, timestamp);
    if (!bgra_camera_frame_opt.has_value()) return base::nullopt;

    CameraFrame bgra_camera_frame = std::move(bgra_camera_frame_opt.value());
    const uint8_t* bgra_data = bgra_camera_frame.data_ptr();
    int width = bgra_camera_frame.width();
    int height = bgra_camera_frame.height();
    CameraFormat camera_format = bgra_camera_frame.camera_format();
    camera_format.set_pixel_format(requested_pixel_format);
    size_t length = camera_format.AllocationSize();
    std::unique_ptr<uint8_t> tmp_camera_frame(new uint8_t[length]);

    int ret = -1;
    switch (requested_pixel_format) {
      case PIXEL_FORMAT_I420:
        ret = libyuv::ARGBToI420(
            bgra_data, width * 4, tmp_camera_frame.get(), width,
            tmp_camera_frame.get() + (width * height), width / 2,
            tmp_camera_frame.get() + (width * height) * 5 / 4, width / 2, width,
            height);
        break;
      case PIXEL_FORMAT_YV12:
        // No conversion in libyuv api.
        break;
      case PIXEL_FORMAT_NV12:
        ret = libyuv::ARGBToNV12(
            bgra_data, width * 4, tmp_camera_frame.get(), width,
            tmp_camera_frame.get() + (width * height), width, width, height);
        break;
      case PIXEL_FORMAT_NV21:
        ret = libyuv::ARGBToNV21(
            bgra_data, width * 4, tmp_camera_frame.get(), width,
            tmp_camera_frame.get() + (width * height), width, width, height);
        break;
      case PIXEL_FORMAT_UYVY:
        ret = libyuv::ARGBToUYVY(bgra_data, width * 4, tmp_camera_frame.get(),
                                 width * 2, width, height);
        break;
      case PIXEL_FORMAT_YUY2:
        ret = libyuv::ARGBToYUY2(bgra_data, width * 4, tmp_camera_frame.get(),
                                 width * 2, width, height);
        break;
      case PIXEL_FORMAT_BGR:
        ret = libyuv::ARGBToRGB24(bgra_data, width * 4, tmp_camera_frame.get(),
                                  width * 3, width, height);
        break;
      case PIXEL_FORMAT_RGBA:
        ret = libyuv::ARGBToABGR(bgra_data, width * 4, tmp_camera_frame.get(),
                                 width * 4, width, height);
        break;
      case PIXEL_FORMAT_RGB:
        ret = libyuv::ARGBToRAW(bgra_data, width * 4, tmp_camera_frame.get(),
                                width * 3, width, height);
        break;
      case PIXEL_FORMAT_ARGB:
        ret = libyuv::ARGBToBGRA(bgra_data, width * 4, tmp_camera_frame.get(),
                                 width * 4, width, height);
        break;
      default:
        break;
    }
    if (ret != 0) return base::nullopt;

    return CameraFrame(std::move(tmp_camera_frame), length, camera_format,
                       timestamp);
  }
}

}  // namespace felicia