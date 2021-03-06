// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import { SizeMessage, SizeMessageProtobuf } from './geometry';
import { PixelFormatProtobuf } from './ui';

export const CAMERA_FORMAT_MESSAGE = 'felicia.drivers.CameraFormatMessage';

export interface CameraFormatMessageProtobuf {
  size: SizeMessageProtobuf;
  pixelFormat: PixelFormatProtobuf;
  frameRate: number;
}

export default class CameraFormatMessage {
  size: SizeMessage;

  pixelFormat: PixelFormatProtobuf;

  frameRate: number;

  constructor({ size, pixelFormat, frameRate }: CameraFormatMessageProtobuf) {
    this.size = new SizeMessage(size);
    this.pixelFormat = pixelFormat;
    this.frameRate = frameRate;
  }
}
