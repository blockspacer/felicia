// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Modified by Wonyong Kim(chokobole33@gmail.com)
// Followings are taken and modified from
// https://github.com/chromium/chromium/blob/5db095c2653f332334d56ad739ae5fe1053308b1/media/capture/video/win/video_capture_device_mf_win.cc
// https://github.com/chromium/chromium/blob/5db095c2653f332334d56ad739ae5fe1053308b1/media/capture/video/win/video_capture_device_factory_win.cc

#include "felicia/drivers/camera/win/mf_camera.h"

#include <mfapi.h>
#include <wrl.h>
#include <wrl/client.h>

#include "third_party/chromium/base/strings/sys_string_conversions.h"
#include "third_party/chromium/base/win/scoped_co_mem.h"

#include "felicia/drivers/camera/camera_errors.h"
#include "felicia/drivers/camera/win/camera_util.h"
#include "felicia/drivers/camera/win/mf_initializer.h"

using base::win::ScopedCoMem;
using Microsoft::WRL::ComPtr;

namespace felicia {

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

}  // namespace

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
    : CameraInterface(camera_descriptor) {}

MfCamera::~MfCamera() {}

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
    return errors::FailedToCreateVideoCaptureDevice();
  }

  ComPtr<IMFSourceReader> reader;
  HRESULT hr = MFCreateSourceReaderFromMediaSource(source.Get(), NULL,
                                                   reader.GetAddressOf());
  if (FAILED(hr)) {
    return errors::FailedToMFCreateSourceReaderFromMediaSource(hr);
  }

  DWORD stream_index = 0;
  ComPtr<IMFMediaType> type;
  while (SUCCEEDED(hr = reader->GetNativeMediaType(
                       static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                       stream_index, type.GetAddressOf()))) {
    UINT32 width, height;
    hr = MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr)) {
      return errors::FailedToMFGetAttributeSize(hr);
    }
    CameraFormat camera_format;
    camera_format.SetSize(width, height);

    UINT32 numerator, denominator;
    hr = MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE, &numerator,
                             &denominator);
    if (FAILED(hr)) {
      return errors::FailedToMFGetAttributeRatio(hr);
    }
    camera_format.set_frame_rate(
        denominator ? static_cast<float>(numerator) / denominator : 0.0f);

    GUID type_guid;
    hr = type->GetGUID(MF_MT_SUBTYPE, &type_guid);
    if (FAILED(hr)) {
      return errors::FailedToGetGUID(hr);
    }
    camera_format.set_pixel_format(CameraFormat::FromMfMediaSubtype(type_guid));
    type.Reset();
    ++stream_index;
    if (camera_format.pixel_format() == CameraFormat::PIXEL_FORMAT_UNKNOWN)
      continue;

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
  return errors::Unimplemented("Not implemented yet.");
}

Status MfCamera::Start(CameraFrameCallback camera_frame_callback,
                       StatusCallback status_callback) {
  return errors::Unimplemented("Not implemented yet.");
}

Status MfCamera::Stop() {
  return errors::Unimplemented("Not implemented yet.");
}

StatusOr<CameraFormat> MfCamera::GetCurrentCameraFormat() {
  return errors::Unimplemented("Not implemented yet.");
}

Status MfCamera::SetCameraFormat(const CameraFormat& camera_format) {
  return errors::Unimplemented("Not implemented yet.");
}

}  // namespace felicia