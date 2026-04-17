// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RawMetadataLoader.h"

#include "ImageMetadata.h"
#include "RawImageLoader.h"
#include "TiffReader.h"
#include "config.h"

#ifdef HAVE_LIBRAW
#include <libraw/libraw.h>
#endif
#include <tiff.h>
#include <tiffio.h>

#include <QFile>
#include <QFileInfo>

namespace {

// RAW and camera DNG files often omit physical resolution tags entirely.
// Returning a stable import-time fallback avoids forcing the Fix DPI step
// when the file simply has no trustworthy DPI metadata to begin with.
const Dpi kRawFallbackDpi(300, 300);

ImageMetadata withValidDpi(const ImageMetadata& metadata) {
  if (!metadata.dpi().isNull()) {
    return metadata;
  }

  ImageMetadata normalized(metadata);
  normalized.setDpi(kRawFallbackDpi);
  return normalized;
}

uint16_t dngOrientationTag(const QString& filePath) {
  const QByteArray encoded = QFile::encodeName(filePath);
  TIFF* tif = TIFFOpen(encoded.constData(), "rm");
  if (!tif) {
    return ORIENTATION_TOPLEFT;
  }

  uint16_t orientation = ORIENTATION_TOPLEFT;
  TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);
  TIFFClose(tif);
  return orientation;
}

bool swapsAxes(const uint16_t orientation) {
  switch (orientation) {
    case ORIENTATION_LEFTTOP:
    case ORIENTATION_RIGHTTOP:
    case ORIENTATION_RIGHTBOT:
    case ORIENTATION_LEFTBOT:
      return true;
    default:
      return false;
  }
}

}  // namespace

ImageMetadataLoader::Status RawMetadataLoader::loadMetadata(
    QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out) {
  QFile* const file = qobject_cast<QFile*>(&ioDevice);
  if (!file || !RawImageLoader::isRawFile(file->fileName())) {
    return FORMAT_NOT_RECOGNIZED;
  }

  const QString suffix = QFileInfo(file->fileName()).suffix().toLower();
  ImageMetadata dngTiffMetadata;
  bool haveDngTiffMetadata = false;
  if (suffix == QLatin1String("dng")) {
    file->seek(0);
    const auto firstMetadataCallback = [&](const ImageMetadata& currentMetadata) {
      if (!haveDngTiffMetadata) {
        dngTiffMetadata = currentMetadata;
        haveDngTiffMetadata = true;
      }
    };
    const ProxyFunction<decltype(firstMetadataCallback), void, const ImageMetadata&> firstMetadataSink(
        firstMetadataCallback);
    TiffReader::readMetadata(ioDevice, firstMetadataSink);
    file->seek(0);
  }

#ifdef HAVE_LIBRAW
  LibRaw raw;
  const QByteArray encoded = QFile::encodeName(file->fileName());
  if (raw.open_file(encoded.constData()) != LIBRAW_SUCCESS) {
    return GENERIC_ERROR;
  }
  if (raw.unpack() != LIBRAW_SUCCESS) {
    return GENERIC_ERROR;
  }

  const int width = raw.imgdata.sizes.width > 0 ? raw.imgdata.sizes.width : raw.imgdata.sizes.iwidth;
  const int height = raw.imgdata.sizes.height > 0 ? raw.imgdata.sizes.height : raw.imgdata.sizes.iheight;
  if (width <= 0 || height <= 0) {
    if (haveDngTiffMetadata) {
      out(withValidDpi(dngTiffMetadata));
      return LOADED;
    }
    return NO_IMAGES;
  }

  QSize imageSize(width, height);
  Dpi dpi = haveDngTiffMetadata && !dngTiffMetadata.dpi().isNull() ? dngTiffMetadata.dpi() : kRawFallbackDpi;
  if (suffix == QLatin1String("dng") && swapsAxes(dngOrientationTag(file->fileName()))) {
    imageSize.transpose();
    dpi = Dpi(dpi.vertical(), dpi.horizontal());
  }

  out(ImageMetadata(imageSize, dpi));
  return LOADED;
#else
  Q_UNUSED(out);
  return GENERIC_ERROR;
#endif
}
