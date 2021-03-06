// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* eslint import/prefer-default-export: "off" */
/* eslint no-param-reassign: ["error", { "props": true, "ignorePropertyModificationsFor": ["pixels"] }] */
import { PixelFormat, PixelFormatProtobuf } from '@felicia-viz/proto/messages/ui';

export interface Color3Indexes {
  rIdx: number;
  gIdx: number;
  bIdx: number;
}

export interface Color4Indexes {
  rIdx: number;
  gIdx: number;
  bIdx: number;
  aIdx: number;
}

export const RGBA: Color4Indexes = {
  rIdx: 0,
  gIdx: 1,
  bIdx: 2,
  aIdx: 3,
};
Object.freeze(RGBA);

export const RGBX: Color4Indexes = {
  rIdx: 0,
  gIdx: 1,
  bIdx: 2,
  aIdx: -1,
};
Object.freeze(RGBX);

export const RGB: Color3Indexes = {
  rIdx: 0,
  gIdx: 1,
  bIdx: 2,
};
Object.freeze(RGB);

export const BGRA: Color4Indexes = {
  rIdx: 2,
  gIdx: 1,
  bIdx: 0,
  aIdx: 3,
};
Object.freeze(BGRA);

export const BGRX: Color4Indexes = {
  rIdx: 2,
  gIdx: 1,
  bIdx: 0,
  aIdx: -1,
};
Object.freeze(BGRX);

export const BGR: Color3Indexes = {
  rIdx: 2,
  gIdx: 1,
  bIdx: 0,
};
Object.freeze(BGR);

export const ARGB: Color4Indexes = {
  rIdx: 1,
  gIdx: 2,
  bIdx: 3,
  aIdx: 0,
};
Object.freeze(ARGB);

function fillPixelsImpl(
  pixels: Uint8ClampedArray,
  width: number,
  height: number,
  data: Uint8Array,
  colorIndexes: Color3Indexes | Color4Indexes
): void {
  const pixelData = new Uint8ClampedArray(data);
  const size = width * height;
  if ((colorIndexes as Color4Indexes).aIdx !== undefined) {
    if ((colorIndexes as Color4Indexes).aIdx < 0) {
      for (let i = 0; i < size; i += 1) {
        const pixelsIdx = i << 2;
        const pixelDataIdx = pixelsIdx;
        pixels[pixelsIdx + RGBA.rIdx] = pixelData[pixelDataIdx + colorIndexes.rIdx];
        pixels[pixelsIdx + RGBA.gIdx] = pixelData[pixelDataIdx + colorIndexes.gIdx];
        pixels[pixelsIdx + RGBA.bIdx] = pixelData[pixelDataIdx + colorIndexes.bIdx];
      }
    } else {
      for (let i = 0; i < size; i += 1) {
        const pixelsIdx = i << 2;
        const pixelDataIdx = pixelsIdx;
        pixels[pixelsIdx + RGBA.rIdx] = pixelData[pixelDataIdx + colorIndexes.rIdx];
        pixels[pixelsIdx + RGBA.gIdx] = pixelData[pixelDataIdx + colorIndexes.gIdx];
        pixels[pixelsIdx + RGBA.bIdx] = pixelData[pixelDataIdx + colorIndexes.bIdx];
        pixels[pixelsIdx + RGBA.aIdx] =
          pixelData[pixelDataIdx + (colorIndexes as Color4Indexes).aIdx];
      }
    }
  } else {
    for (let i = 0; i < size; i += 1) {
      const pixelsIdx = i << 2;
      const pixelDataIdx = i * 3;
      pixels[pixelsIdx + RGBA.rIdx] = pixelData[pixelDataIdx + colorIndexes.rIdx];
      pixels[pixelsIdx + RGBA.gIdx] = pixelData[pixelDataIdx + colorIndexes.gIdx];
      pixels[pixelsIdx + RGBA.bIdx] = pixelData[pixelDataIdx + colorIndexes.bIdx];
      pixels[pixelsIdx + RGBA.aIdx] = 255;
    }
  }
}

function fillGreyPixelsImpl(
  pixels: Uint8ClampedArray,
  width: number,
  height: number,
  data: Uint8Array,
  is8bit: boolean
): void {
  const size = width * height;
  if (is8bit) {
    const pixelData = new Uint8ClampedArray(data);
    for (let i = 0; i < size; i += 1) {
      const pixelsIdx = i << 2;
      pixels[pixelsIdx + RGBA.rIdx] = pixelData[i];
      pixels[pixelsIdx + RGBA.gIdx] = pixelData[i];
      pixels[pixelsIdx + RGBA.bIdx] = pixelData[i];
      pixels[pixelsIdx + RGBA.aIdx] = 255;
    }
  } else {
    const pixelData = new Uint16Array(data);
    for (let i = 0; i < size; i += 1) {
      const pixelsIdx = i << 2;
      const v = Math.round(pixelData[i] / 256);
      pixels[pixelsIdx + RGBA.rIdx] = v;
      pixels[pixelsIdx + RGBA.gIdx] = v;
      pixels[pixelsIdx + RGBA.bIdx] = v;
      pixels[pixelsIdx + RGBA.aIdx] = 255;
    }
  }
}

export function fillPixels(
  pixels: Uint8ClampedArray,
  width: number,
  height: number,
  data: Uint8Array,
  pixelFormat: PixelFormatProtobuf
): boolean {
  let colorIndexes = null;
  if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_BGRA) {
    colorIndexes = BGRA;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_BGR) {
    colorIndexes = BGR;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_BGRX) {
    colorIndexes = BGRX;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_RGBA) {
    colorIndexes = RGBA;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_RGBX) {
    colorIndexes = RGBX;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_RGB) {
    colorIndexes = RGB;
  } else if (pixelFormat === PixelFormat.values.PIXEL_FORMAT_ARGB) {
    colorIndexes = ARGB;
  } else if (
    pixelFormat === PixelFormat.values.PIXEL_FORMAT_Y8 ||
    pixelFormat === PixelFormat.values.PIXEL_FORMAT_Y16
  ) {
    fillGreyPixelsImpl(
      pixels,
      width,
      height,
      data,
      pixelFormat === PixelFormat.values.PIXEL_FORMAT_Y8
    );
    return true;
  } else {
    console.error(`To draw, you need to convert to BGRA format: ${pixelFormat}`);
    return false;
  }

  fillPixelsImpl(pixels, width, height, data, colorIndexes);
  return true;
}
