#include "felicia/slam/dataset/tum_dataset_loader.h"

#include "felicia/slam/dataset/dataset_utils.h"

namespace felicia {
namespace slam {

TumDatasetLoader::TumDatasetLoader(const base::FilePath& path, TumKind kind,
                                   DataKind data_kind)
    : path_(path),
      kind_(kind),
      data_kind_(data_kind),
      current_(0) {
  path_to_data_ = PathToData();
}

StatusOr<SensorMetaData> TumDatasetLoader::Init() {
  SensorMetaData sensor_meta_data;
  if (kind_ == FR1) {
    sensor_meta_data.set_left_K(EigenCameraMatrixd(517.3, 516.5, 318.6, 255.3));
    sensor_meta_data.set_left_D(
        EigenDistortionMatrixd(0.2624, -0.9531, -0.0054, 0.0026, 1.1633));
  } else if (kind_ == FR2) {
    sensor_meta_data.set_left_K(EigenCameraMatrixd(520.9, 521.0, 325.1, 249.7));
    sensor_meta_data.set_left_D(
        EigenDistortionMatrixd(0.2312, -0.7849, -0.0033, -0.0001, 0.9172));
  } else if (kind_ == FR3) {
    sensor_meta_data.set_left_K(EigenCameraMatrixd(535.4, 539.2, 320.1, 247.6));
    sensor_meta_data.set_left_D(
        EigenDistortionMatrixd(0.0, 0.0, 0.0, 0.0, 0.0));
  }

  return sensor_meta_data;
}

StatusOr<SensorData> TumDatasetLoader::Next() {
  if (!reader_.IsOpened()) {
    int skip_header = 3;
    if (data_kind_ != RGBD) {
      skip_header = 0;
    }
    Status s = reader_.Open(path_to_data_, " ", skip_header);
    if (!s.ok()) return s;
  }

  ++current_;
  int current_line = current_ + 3;
  std::vector<std::string> items;
  SensorData sensor_data;
  if (reader_.ReadItems(&items)) {
    if (items.size() != ColumnsForData()) {
      return errors::InvalidArgument(
          base::StringPrintf("The number of columns is not valid at %s:%d",
                             path_to_data_.value().c_str(), current_line));
    }
    StatusOr<double> status_or =
        TryConvertToDouble(items[0], path_to_data_, current_line);
    if (!status_or.ok()) return status_or.status();
    sensor_data.set_timestamp(status_or.ValueOrDie());
    switch (data_kind_) {
      case RGB: {
        sensor_data.set_left_image_filename(
            path_.AppendASCII(items[1]).value());
        break;
      }
      case DEPTH: {
        sensor_data.set_depth_image_filename(
            path_.AppendASCII(items[2]).value());
        break;
      }
      case RGBD: {
        sensor_data.set_left_image_filename(
            path_.AppendASCII(items[1]).value());
        sensor_data.set_depth_image_filename(
            path_.AppendASCII(items[3]).value());
        break;
      }
      case ACCELERATION: {
        float v[3];
        for (int i = 0; i < 3; ++i) {
          status_or =
              TryConvertToDouble(items[i + 1], path_to_data_, current_line);
          if (!status_or.ok()) return status_or.status();
          v[i] = status_or.ValueOrDie();
        }
        sensor_data.set_acceleration(Vector3f{v[0], v[1], v[2]});
        break;
      }
      case GROUND_TRUTH: {
        float v[7];
        for (int i = 0; i < 7; ++i) {
          status_or =
              TryConvertToDouble(items[i + 1], path_to_data_, current_line);
          if (!status_or.ok()) return status_or.status();
          v[i] = status_or.ValueOrDie();
        }
        Point3f p(v[0], v[1], v[2]);
        Quaternionf q(v[3], v[4], v[5], v[6]);
        sensor_data.set_pose(Pose3f{p, q});
        break;
      }
    }
  }
  return sensor_data;
}

bool TumDatasetLoader::End() const { return reader_.eof(); }

base::FilePath TumDatasetLoader::PathToData() const {
  switch (data_kind_) {
    case RGB:
      return path_.AppendASCII("rgb.txt");
    case DEPTH:
      return path_.AppendASCII("depth.txt");
    case RGBD:
      return path_.AppendASCII("associated.txt");
    case ACCELERATION:
      return path_.AppendASCII("accelerometer.txt");
    case GROUND_TRUTH:
      return path_.AppendASCII("groundtruth.txt");
  }
  NOTREACHED();
  return path_;
}

int TumDatasetLoader::ColumnsForData() const {
  switch (data_kind_) {
    case RGB:
      return 2;
    case DEPTH:
      return 2;
    case RGBD:
      return 4;
    case ACCELERATION:
      return 4;
    case GROUND_TRUTH:
      return 8;
  }
  NOTREACHED();
  return 0;
}

}  // namespace slam
}  // namespace felicia