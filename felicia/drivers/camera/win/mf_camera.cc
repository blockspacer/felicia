// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Modified by Wonyong Kim(chokobole33@gmail.com)
// Followings are taken and modified from
// https://github.com/chromium/chromium/blob/5db095c2653f332334d56ad739ae5fe1053308b1/media/capture/video/win/video_capture_device_mf_win.cc
// https://github.com/chromium/chromium/blob/5db095c2653f332334d56ad739ae5fe1053308b1/media/capture/video/win/video_capture_device_factory_win.cc

#include "felicia/drivers/camera/win/mf_camera.h"

#include <mfapi.h>
#include <mferror.h>

#include "third_party/chromium/base/strings/sys_string_conversions.h"
#include "third_party/chromium/base/win/scoped_co_mem.h"

#include "felicia/drivers/camera/camera_errors.h"
#include "felicia/drivers/camera/timestamp_constants.h"
#include "felicia/drivers/camera/win/camera_util.h"
#include "felicia/drivers/camera/win/mf_initializer.h"

using base::win::ScopedCoMem;
using Microsoft::WRL::ComPtr;

namespace felicia {
namespace drivers {

#define MESSAGE_WITH_HRESULT(text, hr) \
  base::StringPrintf("%s :%s.", text,  \
                     ::logging::SystemErrorCodeToString(hr).c_str())

namespace {

const std::vector<std::pair<GUID, GUID>> kAttributes = {
    {MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
     MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID},
};

bool LoadMediaFoundationDlls() {
  static const wchar_t* const kMfDLLs[] = {
      L"%WINDIR%\\system32\\mf.dll", L"%WINDIR%\\system32\\mfplat.dll",
      L"%WINDIR%\\system32\\mfreadwrite.dll",
      L"%WINDIR%\\system32\\MFCaptureEngine.dll"};

  for (const wchar_t* kMfDLL : kMfDLLs) {
    wchar_t path[MAX_PATH] = {0};
    ExpandEnvironmentStringsW(kMfDLL, path, base::size(path));
    if (!LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH))
      return false;
  }
  return true;
}

bool PrepareVideoCaptureAttributesMediaFoundation(
    const std::vector<std::pair<GUID, GUID>>& attributes_data, int count,
    IMFAttributes** attributes) {
  if (!MfCamera::PlatformSupportsMediaFoundation() ||
      !InitializeMediaFoundation()) {
    return false;
  }

  if (FAILED(MFCreateAttributes(attributes, count))) return false;

  for (const auto& value : attributes_data) {
    if (!SUCCEEDED((*attributes)->SetGUID(value.first, value.second)))
      return false;
  }
  return true;
}

bool EnumerateVideoDevicesMediaFoundation(
    const std::vector<std::pair<GUID, GUID>>& attributes_data,
    IMFActivate*** devices, UINT32* count) {
  ComPtr<IMFAttributes> attributes;
  if (!PrepareVideoCaptureAttributesMediaFoundation(
          attributes_data, attributes_data.size(), attributes.GetAddressOf())) {
    return false;
  }
  return SUCCEEDED(MFEnumDeviceSources(attributes.Get(), devices, count));
}

bool CameraDescriptorsContainDeviceId(
    const CameraDescriptors& camera_descriptors, const std::string& device_id) {
  return std::find_if(camera_descriptors.begin(), camera_descriptors.end(),
                      [device_id](const CameraDescriptor& camera_descriptor) {
                        return device_id == camera_descriptor.device_id();
                      }) != camera_descriptors.end();
}

bool CreateVideoCaptureDeviceMediaFoundation(
    const CameraDescriptor& camera_descriptor, IMFMediaSource** source) {
  ComPtr<IMFAttributes> attributes;
  // We allocate kAttributes.size() + 1 (+1 is for sym_link below) elements
  // in attributes store.
  if (!PrepareVideoCaptureAttributesMediaFoundation(
          kAttributes, kAttributes.size() + 1, attributes.GetAddressOf())) {
    return false;
  }

  attributes->SetString(
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
      base::SysUTF8ToWide(camera_descriptor.device_id()).c_str());
  return SUCCEEDED(MFCreateDeviceSource(attributes.Get(), source));
}

HRESULT CreateCaptureEngine(IMFCaptureEngine** engine) {
  ComPtr<IMFCaptureEngineClassFactory> capture_engine_class_factory;
  HRESULT hr = CoCreateInstance(
      CLSID_MFCaptureEngineClassFactory, NULL, CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(capture_engine_class_factory.GetAddressOf()));
  if (FAILED(hr)) return hr;

  return capture_engine_class_factory->CreateInstance(CLSID_MFCaptureEngine,
                                                      IID_PPV_ARGS(engine));
}

// Calculate sink subtype based on source subtype. |passthrough| is set when
// sink and source are the same and means that there should be no transcoding
// done by IMFCaptureEngine.
HRESULT GetMFSinkMediaSubtype(IMFMediaType* source_media_type,
                              GUID* mf_sink_media_subtype, bool* passthrough) {
  GUID source_subtype;
  HRESULT hr = source_media_type->GetGUID(MF_MT_SUBTYPE, &source_subtype);
  if (FAILED(hr)) return hr;

  if (!CameraFormat::ToMfSinkMediaSubtype(source_subtype,
                                          mf_sink_media_subtype))
    return E_FAIL;
  *passthrough = (*mf_sink_media_subtype == source_subtype);
  return S_OK;
}

HRESULT CopyAttribute(IMFAttributes* source_attributes,
                      IMFAttributes* destination_attributes, const GUID& key) {
  PROPVARIANT var;
  PropVariantInit(&var);
  HRESULT hr = source_attributes->GetItem(key, &var);
  if (FAILED(hr)) return hr;

  hr = destination_attributes->SetItem(key, var);
  PropVariantClear(&var);
  return hr;
}

HRESULT ConvertToVideoSinkMediaType(IMFMediaType* source_media_type,
                                    IMFMediaType* sink_media_type) {
  HRESULT hr = sink_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr)) return hr;

  bool passthrough = false;
  GUID mf_sink_media_subtype;
  hr = GetMFSinkMediaSubtype(source_media_type, &mf_sink_media_subtype,
                             &passthrough);
  if (FAILED(hr)) return hr;

  hr = sink_media_type->SetGUID(MF_MT_SUBTYPE, mf_sink_media_subtype);
  // Copying attribute values for passthrough mode is redundant, since the
  // format is kept unchanged, and causes AddStream error MF_E_INVALIDMEDIATYPE.
  if (FAILED(hr) || passthrough) return hr;

  hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_SIZE);
  if (FAILED(hr)) return hr;

  hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_RATE);
  if (FAILED(hr)) return hr;

  hr = CopyAttribute(source_media_type, sink_media_type,
                     MF_MT_PIXEL_ASPECT_RATIO);
  if (FAILED(hr)) return hr;

  return CopyAttribute(source_media_type, sink_media_type,
                       MF_MT_INTERLACE_MODE);
}

}  // namespace

class MFVideoCallback final
    : public base::RefCountedThreadSafe<MFVideoCallback>,
      public IMFCaptureEngineOnSampleCallback,
      public IMFCaptureEngineOnEventCallback {
 public:
  MFVideoCallback(MfCamera* observer) : observer_(observer) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** object) override {
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IUnknown) {
      *object = this;
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnSampleCallback) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnEventCallback) {
      *object = static_cast<IMFCaptureEngineOnEventCallback*>(this);
      hr = S_OK;
    }
    if (SUCCEEDED(hr)) AddRef();

    return hr;
  }

  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<MFVideoCallback>::AddRef();
    return 1U;
  }

  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<MFVideoCallback>::Release();
    return 1U;
  }

  STDMETHOD(OnEvent)(IMFMediaEvent* media_event) override {
    observer_->OnEvent(media_event);
    return S_OK;
  }

  STDMETHOD(OnSample)(IMFSample* sample) override {
    if (!sample) {
      observer_->OnFrameDropped(
          Status(error::Code::DATA_LOSS, "Received sample is null."));
      return S_OK;
    }

    base::TimeTicks reference_time(base::TimeTicks::Now());
    LONGLONG raw_time_stamp = 0;
    sample->GetSampleTime(&raw_time_stamp);
    base::TimeDelta timestamp =
        base::TimeDelta::FromMicroseconds(raw_time_stamp / 10);

    DWORD count = 0;
    sample->GetBufferCount(&count);

    for (DWORD i = 0; i < count; ++i) {
      ComPtr<IMFMediaBuffer> buffer;
      sample->GetBufferByIndex(i, buffer.GetAddressOf());
      if (buffer) {
        DWORD length = 0, max_length = 0;
        BYTE* data = NULL;
        buffer->Lock(&data, &max_length, &length);
        if (data) {
          observer_->OnIncomingCapturedData(data, length, reference_time,
                                            timestamp);
        } else {
          observer_->OnFrameDropped(Status(
              error::Code::DATA_LOSS, "Locking buffer delievered nullptr."));
        }
        buffer->Unlock();
      } else {
        observer_->OnFrameDropped(
            Status(error::Code::DATA_LOSS, "GetBufferByIndex returned null."));
      }
    }
    return S_OK;
  }

 private:
  friend class base::RefCountedThreadSafe<MFVideoCallback>;
  ~MFVideoCallback() {}
  MfCamera* observer_;
};

// Returns true if the current platform supports the Media Foundation API
// and that the DLLs are available.  On Vista this API is an optional download
// but the API is advertised as a part of Windows 7 and onwards.  However,
// we've seen that the required DLLs are not available in some Win7
// distributions such as Windows 7 N and Windows 7 KN.
// static
bool MfCamera::PlatformSupportsMediaFoundation() {
  static bool g_dlls_available = LoadMediaFoundationDlls();
  return g_dlls_available;
}

MfCamera::MfCamera(const CameraDescriptor& camera_descriptor)
    : CameraInterface(camera_descriptor),
      max_retry_count_(200),
      retry_delay_in_ms_(50) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

MfCamera::~MfCamera() { DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_); }

// static
Status MfCamera::GetCameraDescriptors(CameraDescriptors* camera_descriptors) {
  DCHECK(camera_descriptors);
  ScopedCoMem<IMFActivate*> devices;
  UINT32 count;
  if (!EnumerateVideoDevicesMediaFoundation(kAttributes, &devices, &count)) {
    return errors::FailedToGetCameraDescriptors();
  }
  const bool list_was_empty = !camera_descriptors->size();
  for (UINT32 i = 0; i < count; ++i) {
    ScopedCoMem<wchar_t> name;
    UINT32 name_size;
    HRESULT hr = devices[i]->GetAllocatedString(
        MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &name_size);
    if (SUCCEEDED(hr)) {
      ScopedCoMem<wchar_t> id;
      UINT32 id_size;
      hr = devices[i]->GetAllocatedString(
          MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &id,
          &id_size);
      if (SUCCEEDED(hr)) {
        const std::string device_id =
            base::SysWideToUTF8(std::wstring(id, id_size));
        const std::string model_id = GetDeviceModelId(device_id);
        if (list_was_empty ||
            !CameraDescriptorsContainDeviceId(*camera_descriptors, device_id)) {
          camera_descriptors->emplace_back(
              base::SysWideToUTF8(std::wstring(name, name_size)), device_id,
              model_id);
        }
      }
    }
    DLOG_IF(ERROR, FAILED(hr)) << "GetAllocatedString failed: "
                               << logging::SystemErrorCodeToString(hr);
    devices[i]->Release();
  }

  return Status::OK();
}

// static
Status MfCamera::GetSupportedCameraFormats(
    const CameraDescriptor& camera_descriptor, CameraFormats* camera_formats) {
  DCHECK(camera_formats->empty());

  ComPtr<IMFMediaSource> source;
  if (!CreateVideoCaptureDeviceMediaFoundation(camera_descriptor,
                                               source.GetAddressOf())) {
    return felicia::errors::Unavailable(
        "Failed to create video capture device.");
  }

  ComPtr<IMFSourceReader> reader;
  HRESULT hr = MFCreateSourceReaderFromMediaSource(source.Get(), NULL,
                                                   reader.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(MESSAGE_WITH_HRESULT(
        "Failed to MFCreateSourceReaderFromMediaSource", hr));
  }

  DWORD stream_index = 0;
  ComPtr<IMFMediaType> type;
  while (SUCCEEDED(hr = reader->GetNativeMediaType(
                       static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                       stream_index, type.GetAddressOf()))) {
    CameraFormat camera_format;
    Status s = MfCamera::GetCameraFormatFromSourceMediaType(type.Get(), false,
                                                            &camera_format);
    if (!s.ok()) return s;

    type.Reset();
    ++stream_index;

    if (camera_format.pixel_format() == PIXEL_FORMAT_UNKNOWN) continue;

    if (std::find(camera_formats->begin(), camera_formats->end(),
                  camera_format) == camera_formats->end()) {
      camera_formats->push_back(camera_format);
    }

    DVLOG(1) << camera_descriptor.display_name() << " "
             << camera_format.ToString();
  }

  return Status::OK();
}

Status MfCamera::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!camera_state_.IsStopped()) {
    return camera_state_.InvalidStateError();
  }

  HRESULT hr = CreateCaptureEngine(engine_.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to create CaptureEngine", hr));
  }

  ComPtr<IMFAttributes> attributes;
  MFCreateAttributes(attributes.GetAddressOf(), 1);
  DCHECK(attributes);

  if (!CreateVideoCaptureDeviceMediaFoundation(camera_descriptor_,
                                               source_.GetAddressOf())) {
    return felicia::errors::Unavailable(
        "Failed to create video capture device.");
  }

  video_callback_ = new MFVideoCallback(this);
  hr = engine_->Initialize(video_callback_.get(), attributes.Get(), nullptr,
                           source_.Get());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to Initialize", hr));
  }

  camera_state_.ToInitialized();

  return Status::OK();
}

Status MfCamera::Start(const CameraFormat& requested_camera_format,
                       CameraFrameCallback camera_frame_callback,
                       StatusCallback status_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::AutoLock lock(lock_);

  if (!camera_state_.IsInitialized()) {
    return camera_state_.InvalidStateError();
  }

  ComPtr<IMFCaptureSource> source;
  HRESULT hr = engine_->GetSource(source.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to GetSource", hr));
  }

  CapabilityList video_capabilities;
  hr = FillCapabilities(source.Get(), false, &video_capabilities);
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to FillVideoCapabilities", hr));
  }

  if (video_capabilities.empty()) {
    return errors::NoVideoCapbility();
  }

  const Capability& found_capability =
      GetBestMatchedCapability(requested_camera_format, video_capabilities);

  ComPtr<IMFMediaType> source_video_media_type;
  hr = GetAvailableDeviceMediaType(source.Get(), found_capability.stream_index,
                                   found_capability.media_type_index,
                                   source_video_media_type.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to GetAvailableDeviceMediaType", hr));
  }

  hr = source->SetCurrentDeviceMediaType(found_capability.stream_index,
                                         source_video_media_type.Get());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to SetCurrentDeviceMediaType", hr));
  }

  requested_pixel_format_ = requested_camera_format.pixel_format();
  camera_format_ = found_capability.supported_format;

  ComPtr<IMFCaptureSink> sink;
  hr = engine_->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW,
                        sink.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to GetSink", hr));
  }

  ComPtr<IMFCapturePreviewSink> preview_sink;
  hr = sink->QueryInterface(IID_PPV_ARGS(preview_sink.GetAddressOf()));
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(MESSAGE_WITH_HRESULT(
        "Failed to query CapturePreviewSink interface", hr));
  }

  hr = preview_sink->RemoveAllStreams();
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to RemoveAllStreams", hr));
  }

  ComPtr<IMFMediaType> sink_video_media_type;
  hr = MFCreateMediaType(sink_video_media_type.GetAddressOf());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to create SinkVideoMediaType", hr));
  }

  hr = ConvertToVideoSinkMediaType(source_video_media_type.Get(),
                                   sink_video_media_type.Get());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to ConvertToVideoSinkMediaType", hr));
  }

  DWORD dw_sink_stream_index = 0;
  hr = preview_sink->AddStream(found_capability.stream_index,
                               sink_video_media_type.Get(), NULL,
                               &dw_sink_stream_index);
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to AddStream", hr));
  }

  hr = preview_sink->SetSampleCallback(dw_sink_stream_index,
                                       video_callback_.get());
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to SetSampleCallback", hr));
  }

  hr = engine_->StartPreview();
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to StartPreview", hr));
  }

  camera_frame_callback_ = camera_frame_callback;
  status_callback_ = status_callback;

  camera_state_.ToStarted();

  return Status::OK();
}

Status MfCamera::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::AutoLock lock(lock_);

  if (!camera_state_.IsStarted()) {
    return camera_state_.InvalidStateError();
  }

  HRESULT hr = engine_->StopPreview();
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to StopPreview", hr));
  }

  camera_frame_callback_.Reset();
  status_callback_.Reset();

  camera_state_.ToStopped();

  return Status::OK();
}

Status MfCamera::SetCameraSettings(const CameraSettings& camera_settings) {
  if (camera_state_.IsStopped()) {
    return camera_state_.InvalidStateError();
  }

  if (!camera_control_ || !video_control_) {
    Status s = InitializeVideoAndCameraControls();
    if (!s.ok()) return s;
  }

  HRESULT hr;
  if (camera_settings.has_white_balance_mode()) {
    bool is_auto =
        camera_settings.white_balance_mode() == CAMERA_SETTINGS_MODE_AUTO;
    const long value = is_auto ? 1 : 0;
    const long flag =
        is_auto ? VideoProcAmp_Flags_Auto : VideoProcAmp_Flags_Manual;
    hr = video_control_->Set(VideoProcAmp_WhiteBalance, value, flag);
    if (FAILED(hr)) DLOG(ERROR) << "setting whilte_balance_mode to " << value;
  }

  if (camera_settings.has_color_temperature()) {
    bool can_set = false;
    {
      long value, flag;
      hr = video_control_->Get(VideoProcAmp_WhiteBalance, &value, &flag);
      can_set = SUCCEEDED(hr) && flag & VideoProcAmp_Flags_Manual;
    }
    if (can_set) {
      const long value = camera_settings.color_temperature();
      hr = video_control_->Set(VideoProcAmp_WhiteBalance, value,
                               VideoProcAmp_Flags_Manual);
      if (FAILED(hr)) DLOG(ERROR) << "setting color_temperature to " << value;
    }
  }

  if (camera_settings.has_exposure_mode()) {
    bool is_auto = camera_settings.exposure_mode() == CAMERA_SETTINGS_MODE_AUTO;
    const long value = is_auto ? 1 : 0;
    const long flag =
        is_auto ? CameraControl_Flags_Auto : CameraControl_Flags_Manual;
    hr = camera_control_->Set(CameraControl_Exposure, value, flag);
    if (FAILED(hr)) DLOG(ERROR) << "setting exposure_mode to " << value;
  }

  if (camera_settings.has_exposure_compensation()) {
    bool can_set = false;
    {
      long value, flag;
      hr = camera_control_->Get(CameraControl_Exposure, &value, &flag);
      can_set = SUCCEEDED(hr) && flag & CameraControl_Flags_Manual;
    }
    if (can_set) {
      const long value = camera_settings.exposure_compensation();
      hr = camera_control_->Set(CameraControl_Exposure, value,
                                CameraControl_Flags_Manual);
      if (FAILED(hr))
        DLOG(ERROR) << "setting exposure_compensation to " << value;
    }
  }

  if (camera_settings.has_brightness()) {
    const long value = camera_settings.brightness();
    hr = video_control_->Set(VideoProcAmp_Brightness, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting brightness to " << value;
  }

  if (camera_settings.has_contrast()) {
    const long value = camera_settings.contrast();
    hr = video_control_->Set(VideoProcAmp_Contrast, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting contrast to " << value;
  }

  if (camera_settings.has_saturation()) {
    const long value = camera_settings.saturation();
    hr = video_control_->Set(VideoProcAmp_Saturation, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting saturation to " << value;
  }

  if (camera_settings.has_sharpness()) {
    const long value = camera_settings.sharpness();
    hr = video_control_->Set(VideoProcAmp_Sharpness, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting sharpness to " << value;
  }

  if (camera_settings.has_hue()) {
    const long value = camera_settings.hue();
    hr =
        video_control_->Set(VideoProcAmp_Hue, value, VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting hue to " << value;
  }

  if (camera_settings.has_gain()) {
    const long value = camera_settings.gain();
    hr = video_control_->Set(VideoProcAmp_Gain, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting gain to " << value;
  }

  if (camera_settings.has_gamma()) {
    const long value = camera_settings.gamma();
    hr = video_control_->Set(VideoProcAmp_Gamma, value,
                             VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) DLOG(ERROR) << "setting gamma to " << value;
  }

  return Status::OK();
}

Status MfCamera::GetCameraSettingsInfo(
    CameraSettingsInfoMessage* camera_settings) {
  if (camera_state_.IsStopped()) {
    return camera_state_.InvalidStateError();
  }

  if (!camera_control_ || !video_control_) {
    Status s = InitializeVideoAndCameraControls();
    if (!s.ok()) return s;
  }

  GetCameraSetting(VideoProcAmp_WhiteBalance,
                   camera_settings->mutable_white_balance_mode());
  GetCameraSetting(CameraControl_Exposure,
                   camera_settings->mutable_exposure_mode(),
                   true /* camera_control */);
  GetCameraSetting(CameraControl_Exposure,
                   camera_settings->mutable_exposure_compensation(),
                   true /* camera_control */);
  GetCameraSetting(VideoProcAmp_WhiteBalance,
                   camera_settings->mutable_color_temperature());
  GetCameraSetting(VideoProcAmp_Brightness,
                   camera_settings->mutable_brightness());
  GetCameraSetting(VideoProcAmp_Contrast, camera_settings->mutable_contrast());
  GetCameraSetting(VideoProcAmp_Saturation,
                   camera_settings->mutable_saturation());
  GetCameraSetting(VideoProcAmp_Sharpness,
                   camera_settings->mutable_sharpness());
  GetCameraSetting(VideoProcAmp_Hue, camera_settings->mutable_hue());
  GetCameraSetting(VideoProcAmp_Gain, camera_settings->mutable_gain());
  GetCameraSetting(VideoProcAmp_Gamma, camera_settings->mutable_gamma());

  return Status::OK();
}

namespace {

CameraSettingsMode ValueToMode(long flag, bool camera_control) {
  if (camera_control) {
    if (flag & CameraControl_Flags_Auto)
      return CameraSettingsMode::CAMERA_SETTINGS_MODE_AUTO;
    else if (flag & CameraControl_Flags_Manual)
      return CameraSettingsMode::CAMERA_SETTINGS_MODE_MANUAL;
  } else {
    if (flag & VideoProcAmp_Flags_Auto)
      return CameraSettingsMode::CAMERA_SETTINGS_MODE_AUTO;
    else if (flag & VideoProcAmp_Flags_Manual)
      return CameraSettingsMode::CAMERA_SETTINGS_MODE_MANUAL;
  }

  return CameraSettingsMode::CAMERA_SETTINGS_MODE_NONE;
}

}  // namespace

void MfCamera::GetCameraSetting(long property, CameraSettingsModeValue* value,
                                bool camera_control) {
  HRESULT hr;
  long min, max, step, default_, flag, v;
  if (camera_control) {
    hr = camera_control_->GetRange(property, &min, &max, &step, &default_,
                                   &flag);
  } else {
    hr =
        video_control_->GetRange(property, &min, &max, &step, &default_, &flag);
  }
  if (FAILED(hr)) {
    value->Clear();
    return;
  }
  value->add_modes(CameraSettingsMode::CAMERA_SETTINGS_MODE_AUTO);
  value->add_modes(CameraSettingsMode::CAMERA_SETTINGS_MODE_MANUAL);
  value->set_default_(ValueToMode(flag, camera_control));

  if (camera_control) {
    hr = camera_control_->Get(property, &v, &flag);
  } else {
    hr = video_control_->Get(property, &v, &flag);
  }
  if (FAILED(hr)) {
    value->Clear();
    return;
  }
  value->set_current(ValueToMode(flag, camera_control));
}

void MfCamera::GetCameraSetting(long property, CameraSettingsRangedValue* value,
                                bool camera_control) {
  HRESULT hr;
  long min, max, step, default_, flag, v;
  if (camera_control) {
    hr = camera_control_->GetRange(property, &min, &max, &step, &default_,
                                   &flag);
  } else {
    hr =
        video_control_->GetRange(property, &min, &max, &step, &default_, &flag);
  }
  if (FAILED(hr)) {
    value->Clear();
    return;
  }
  value->set_min(static_cast<int64_t>(min));
  value->set_max(static_cast<int64_t>(max));
  value->set_step(static_cast<int64_t>(step));
  value->set_default_(static_cast<int64_t>(default_));
  value->set_flags(static_cast<int64_t>(flag));

  if (camera_control) {
    hr = camera_control_->Get(property, &v, &flag);
  } else {
    hr = video_control_->Get(property, &v, &flag);
  }
  if (FAILED(hr)) {
    value->Clear();
    return;
  }
  value->set_current(static_cast<int64_t>(v));
}

void MfCamera::OnIncomingCapturedData(const uint8_t* data, int length,
                                      base::TimeTicks reference_time,
                                      base::TimeDelta timestamp) {
  base::AutoLock lock(lock_);

  if (camera_format_.pixel_format() != PixelFormat::PIXEL_FORMAT_MJPEG &&
      camera_format_.AllocationSize() != length) {
    status_callback_.Run(errors::InvalidNumberOfBytesInBuffer());
    return;
  }

  if (requested_pixel_format_ == camera_format_.pixel_format()) {
    Data new_data(data, length);
    camera_frame_callback_.Run(
        CameraFrame{std::move(new_data), camera_format_, timestamp});
  } else {
    base::Optional<CameraFrame> camera_frame = ConvertToRequestedPixelFormat(
        data, length, camera_format_, requested_pixel_format_, timestamp);
    if (camera_frame.has_value()) {
      camera_frame_callback_.Run(std::move(camera_frame.value()));
    } else {
      status_callback_.Run(errors::FailedToConvertToRequestedPixelFormat(
          requested_pixel_format_));
    }
  }
}

void MfCamera::OnFrameDropped(Status s) {
  base::AutoLock lock(lock_);
  status_callback_.Run(std::move(s));
}

void MfCamera::OnEvent(IMFMediaEvent* media_event) {
  base::AutoLock lock(lock_);

  HRESULT hr;
  media_event->GetStatus(&hr);

  if (FAILED(hr))
    status_callback_.Run(felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("MediaEventStatusFailed", hr)));
}

HRESULT MfCamera::ExecuteHresultCallbackWithRetries(
    base::RepeatingCallback<HRESULT()> callback) {
  // Retry callback execution on MF_E_INVALIDREQUEST.
  // MF_E_INVALIDREQUEST is not documented in MediaFoundation documentation.
  // It could mean that MediaFoundation or the underlying device can be in a
  // state that reject these calls. Since MediaFoundation gives no intel about
  // that state beginning and ending (i.e. via some kind of event), we retry the
  // call until it succeed.
  HRESULT hr;
  int retry_count = 0;
  do {
    hr = callback.Run();
    if (FAILED(hr))
      base::PlatformThread::Sleep(
          base::TimeDelta::FromMilliseconds(retry_delay_in_ms_));

    // Give up after some amount of time
  } while (hr == MF_E_INVALIDREQUEST && retry_count++ < max_retry_count_);

  return hr;
}

HRESULT MfCamera::GetDeviceStreamCount(IMFCaptureSource* source, DWORD* count) {
  // Sometimes, GetDeviceStreamCount returns an
  // undocumented MF_E_INVALIDREQUEST. Retrying solves the issue.
  return ExecuteHresultCallbackWithRetries(base::BindRepeating(
      [](IMFCaptureSource* source, DWORD* count) {
        return source->GetDeviceStreamCount(count);
      },
      base::Unretained(source), count));
}

HRESULT MfCamera::GetDeviceStreamCategory(
    IMFCaptureSource* source, DWORD stream_index,
    MF_CAPTURE_ENGINE_STREAM_CATEGORY* stream_category) {
  // We believe that GetDeviceStreamCategory could be affected by the same
  // behaviour of GetDeviceStreamCount and GetAvailableDeviceMediaType
  return ExecuteHresultCallbackWithRetries(base::BindRepeating(
      [](IMFCaptureSource* source, DWORD stream_index,
         MF_CAPTURE_ENGINE_STREAM_CATEGORY* stream_category) {
        return source->GetDeviceStreamCategory(stream_index, stream_category);
      },
      base::Unretained(source), stream_index, stream_category));
}

HRESULT MfCamera::GetAvailableDeviceMediaType(IMFCaptureSource* source,
                                              DWORD stream_index,
                                              DWORD media_type_index,
                                              IMFMediaType** type) {
  // Rarely, for some unknown reason, GetAvailableDeviceMediaType returns an
  // undocumented MF_E_INVALIDREQUEST. Retrying solves the issue.
  return ExecuteHresultCallbackWithRetries(base::BindRepeating(
      [](IMFCaptureSource* source, DWORD stream_index, DWORD media_type_index,
         IMFMediaType** type) {
        return source->GetAvailableDeviceMediaType(stream_index,
                                                   media_type_index, type);
      },
      base::Unretained(source), stream_index, media_type_index, type));
}

HRESULT MfCamera::FillCapabilities(IMFCaptureSource* source, bool photo,
                                   CapabilityList* capabilities) {
  DWORD stream_count = 0;
  HRESULT hr = GetDeviceStreamCount(source, &stream_count);
  if (FAILED(hr)) return hr;

  for (DWORD stream_index = 0; stream_index < stream_count; stream_index++) {
    MF_CAPTURE_ENGINE_STREAM_CATEGORY stream_category;
    hr = GetDeviceStreamCategory(source, stream_index, &stream_category);
    if (FAILED(hr)) return hr;

    if ((photo && stream_category !=
                      MF_CAPTURE_ENGINE_STREAM_CATEGORY_PHOTO_INDEPENDENT) ||
        (!photo &&
         stream_category != MF_CAPTURE_ENGINE_STREAM_CATEGORY_VIDEO_PREVIEW &&
         stream_category != MF_CAPTURE_ENGINE_STREAM_CATEGORY_VIDEO_CAPTURE)) {
      continue;
    }

    DWORD media_type_index = 0;
    ComPtr<IMFMediaType> type;
    while (SUCCEEDED(hr = GetAvailableDeviceMediaType(source, stream_index,
                                                      media_type_index,
                                                      type.GetAddressOf()))) {
      CameraFormat camera_format;
      Status s =
          GetCameraFormatFromSourceMediaType(type.Get(), photo, &camera_format);

      if (s.ok() && camera_format.pixel_format() != PIXEL_FORMAT_UNKNOWN) {
        capabilities->emplace_back(media_type_index, camera_format,
                                   stream_index);
      }

      type.Reset();
      ++media_type_index;
    }
    if (hr == MF_E_NO_MORE_TYPES) {
      hr = S_OK;
    }
    if (FAILED(hr)) {
      return hr;
    }
  }

  return hr;
}

Status MfCamera::InitializeVideoAndCameraControls() {
  HRESULT hr =
      source_->QueryInterface(IID_PPV_ARGS(camera_control_.GetAddressOf()));
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to query IAMCameraControl interface", hr));
  }

  hr = source_->QueryInterface(IID_PPV_ARGS(video_control_.GetAddressOf()));
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to query IAMVideoProcAmp interface", hr));
  }

  return Status::OK();
}

// static
Status MfCamera::GetCameraFormatFromSourceMediaType(
    IMFMediaType* source_media_type, bool photo, CameraFormat* camera_format) {
  GUID major_type_guid;
  HRESULT hr = source_media_type->GetGUID(MF_MT_MAJOR_TYPE, &major_type_guid);
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to GetGUID", hr));
  }

  if (major_type_guid != MFMediaType_Image) {
    if (photo)
      return felicia::errors::InvalidArgument("MediaType not matched.");
    UINT32 numerator, denominator;
    hr = MFGetAttributeRatio(source_media_type, MF_MT_FRAME_RATE, &numerator,
                             &denominator);
    if (FAILED(hr)) {
      return felicia::errors::Unavailable(
          MESSAGE_WITH_HRESULT("Failed to MFGetAttributeRatio", hr));
    }
    camera_format->set_frame_rate(
        denominator ? static_cast<float>(numerator) / denominator : 0.0f);
  }

  GUID sub_type_guid;
  hr = source_media_type->GetGUID(MF_MT_SUBTYPE, &sub_type_guid);
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to GetGUID", hr));
  }

  camera_format->set_pixel_format(
      CameraFormat::FromMfMediaSubtype(sub_type_guid));

  UINT32 width, height;
  hr = MFGetAttributeSize(source_media_type, MF_MT_FRAME_SIZE, &width, &height);
  if (FAILED(hr)) {
    return felicia::errors::Unavailable(
        MESSAGE_WITH_HRESULT("Failed to MFGetAttributeSize", hr));
  }
  camera_format->SetSize(width, height);

  return Status::OK();
}

#undef MESSAGE_WITH_HRESULT

}  // namespace drivers
}  // namespace felicia