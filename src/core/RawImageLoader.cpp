// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RawImageLoader.h"

#include "ColorProfileUtils.h"
#include "config.h"

#ifdef HAVE_LIBRAW
#include <libraw/libraw.h>
#endif
#include <tiff.h>
#include <tiffio.h>

#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTransform>
#include <QRgba64>
#include <cstring>

namespace {
QByteArray encodedPath(const QString& path) {
  return QFile::encodeName(path);
}

uint16_t dngOrientationTag(const QString& path) {
  const QFileInfo fileInfo(path);
  if (fileInfo.suffix().compare(QStringLiteral("dng"), Qt::CaseInsensitive) != 0) {
    return ORIENTATION_TOPLEFT;
  }

  const QByteArray encoded = encodedPath(path);
  TIFF* tif = TIFFOpen(encoded.constData(), "rm");
  if (!tif) {
    return ORIENTATION_TOPLEFT;
  }

  uint16_t orientation = ORIENTATION_TOPLEFT;
  TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);
  TIFFClose(tif);
  return orientation;
}

QImage applyOrientation(const QImage& image, const uint16_t orientation) {
  if (image.isNull()) {
    return image;
  }

  switch (orientation) {
    case ORIENTATION_TOPLEFT:
      return image;
    case ORIENTATION_TOPRIGHT:
      return image.mirrored(true, false);
    case ORIENTATION_BOTRIGHT:
      return image.transformed(QTransform().rotate(180));
    case ORIENTATION_BOTLEFT:
      return image.mirrored(false, true);
    case ORIENTATION_LEFTTOP:
      return image.transformed(QTransform().scale(-1.0, 1.0).rotate(90));
    case ORIENTATION_RIGHTTOP:
      return image.transformed(QTransform().rotate(90));
    case ORIENTATION_RIGHTBOT:
      return image.transformed(QTransform().scale(-1.0, 1.0).rotate(-90));
    case ORIENTATION_LEFTBOT:
      return image.transformed(QTransform().rotate(-90));
    default:
      return image;
  }
}
}  // namespace

QStringList RawImageLoader::supportedExtensions() {
#ifdef HAVE_LIBRAW
  return {"dng", "cr2", "cr3", "crw", "nef", "nrw", "arw", "srf", "sr2",
          "raf", "orf", "pef", "ptx", "rw2", "rwl", "srw", "3fr", "fff",
          "iiq", "mrw", "x3f"};
#else
  return {};
#endif
}

bool RawImageLoader::isRawFile(const QString& path) {
  const QString ext = QFileInfo(path).suffix().toLower();
  return supportedExtensions().contains(ext);
}

#ifdef HAVE_LIBRAW
namespace {

void applyParams(LibRaw& raw, const RawLoadParams& params) {
  libraw_output_params_t& ip = raw.imgdata.params;

  ip.user_qual = static_cast<int>(params.demosaic);
  ip.use_camera_wb = 0;
  ip.use_auto_wb = 0;

  switch (params.whiteBalance) {
    case RawLoadParams::WhiteBalance::Camera:
      ip.use_camera_wb = 1;
      break;
    case RawLoadParams::WhiteBalance::Auto:
      ip.use_auto_wb = 1;
      break;
    case RawLoadParams::WhiteBalance::Daylight:
      break;
    case RawLoadParams::WhiteBalance::Manual:
      for (int i = 0; i < 4; ++i) {
        ip.user_mul[i] = params.wbMult[i];
      }
      break;
  }

  ip.use_camera_matrix = params.useCameraMatrix ? 1 : 0;
  ip.output_bps = 16;
  ip.output_color = 1;  // sRGB
  ip.no_auto_bright = 1;
  ip.half_size = params.halfSize ? 1 : 0;

  if (params.exposureShift != 0.0f) {
    ip.exp_correc = 1;
    ip.exp_shift = params.exposureShift;
    ip.exp_preser = 0.0f;
  } else {
    ip.exp_correc = 0;
  }
}

QImage convertBitmapToQImage(const libraw_processed_image_t* image) {
  if (!image || image->type != LIBRAW_IMAGE_BITMAP || image->width <= 0 || image->height <= 0) {
    return QImage();
  }

  const int width = image->width;
  const int height = image->height;
  const int colors = image->colors;
  const int bits = image->bits;

  if ((colors != 3 && colors != 4) || (bits != 8 && bits != 16)) {
    return QImage();
  }

  if (bits == 16) {
    QImage result(width, height, colors == 4 ? QImage::Format_RGBA64 : QImage::Format_RGBX64);
    const quint16* src = reinterpret_cast<const quint16*>(image->data);
    for (int y = 0; y < height; ++y) {
      QRgba64* dst = reinterpret_cast<QRgba64*>(result.scanLine(y));
      for (int x = 0; x < width; ++x) {
        const quint16 r = src[0];
        const quint16 g = src[1];
        const quint16 b = src[2];
        const quint16 a = colors == 4 ? src[3] : 0xffff;
        dst[x] = QRgba64::fromRgba64(r, g, b, a);
        src += colors;
      }
    }
    color_profile::applyDefaultColorProfile(result);
    return result;
  }

  QImage result(width, height, colors == 4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
  const uchar* src = image->data;
  for (int y = 0; y < height; ++y) {
    uchar* dst = result.scanLine(y);
    memcpy(dst, src, static_cast<size_t>(width * colors));
    src += width * colors;
  }
  color_profile::applyDefaultColorProfile(result);
  return result;
}

}  // namespace
#endif

QImage RawImageLoader::load(const QString& path, const RawLoadParams& params) {
#ifdef HAVE_LIBRAW
  LibRaw raw;
  applyParams(raw, params);

  const QByteArray encoded = encodedPath(path);
  if (raw.open_file(encoded.constData()) != LIBRAW_SUCCESS) {
    return QImage();
  }
  if (raw.unpack() != LIBRAW_SUCCESS) {
    return QImage();
  }
  if (raw.dcraw_process() != LIBRAW_SUCCESS) {
    return QImage();
  }

  int errorCode = LIBRAW_SUCCESS;
  libraw_processed_image_t* image = raw.dcraw_make_mem_image(&errorCode);
  if (!image || errorCode != LIBRAW_SUCCESS) {
    if (image) {
      LibRaw::dcraw_clear_mem(image);
    }
    return QImage();
  }

  QImage result = convertBitmapToQImage(image);
  LibRaw::dcraw_clear_mem(image);
  return result;
#else
  Q_UNUSED(path);
  Q_UNUSED(params);
  return QImage();
#endif
}

QImage RawImageLoader::loadThumbnail(const QString& path) {
#ifdef HAVE_LIBRAW
  LibRaw raw;
  const uint16_t containerOrientation = dngOrientationTag(path);
  const bool useContainerOrientation = containerOrientation != ORIENTATION_TOPLEFT;
  const QByteArray encoded = encodedPath(path);
  if (raw.open_file(encoded.constData()) != LIBRAW_SUCCESS) {
    return QImage();
  }
  if (raw.unpack_thumb() != LIBRAW_SUCCESS) {
    return QImage();
  }

  int errorCode = LIBRAW_SUCCESS;
  libraw_processed_image_t* thumb = raw.dcraw_make_mem_thumb(&errorCode);
  if (!thumb || errorCode != LIBRAW_SUCCESS) {
    if (thumb) {
      LibRaw::dcraw_clear_mem(thumb);
    }
    return QImage();
  }

  QImage result;
  if (thumb->type == LIBRAW_IMAGE_JPEG) {
    QByteArray thumbData(reinterpret_cast<const char*>(thumb->data), static_cast<qsizetype>(thumb->data_size));
    QBuffer buffer(&thumbData);
    if (buffer.open(QIODevice::ReadOnly)) {
      QImageReader reader(&buffer, "JPEG");
      if (!useContainerOrientation) {
        // For non-DNG RAW files, embedded preview EXIF orientation is usually
        // the best available hint.
        reader.setAutoTransform(true);
      }
      reader.read(&result);
    }
  } else {
    result = convertBitmapToQImage(thumb);
  }

  LibRaw::dcraw_clear_mem(thumb);
  return useContainerOrientation ? applyOrientation(result, containerOrientation) : result;
#else
  Q_UNUSED(path);
  return QImage();
#endif
}
