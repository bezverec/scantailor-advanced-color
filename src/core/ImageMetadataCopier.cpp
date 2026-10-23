// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageMetadataCopier.h"

#include <QFile>
#include <QSaveFile>

#ifdef SCANTAILOR_HAVE_EXIV2
#include <exiv2/exiv2.hpp>

#include <exception>
#include <mutex>
#include <string>
#endif

namespace image_metadata {
#ifndef SCANTAILOR_HAVE_EXIV2

bool isCopySupported() {
  return false;
}

bool copyMetadata(const QString&, const QString&, QString* errorMessage) {
  if (errorMessage) {
    *errorMessage = QStringLiteral("built without Exiv2");
  }
  return false;
}

#else  // SCANTAILOR_HAVE_EXIV2

namespace {
// Exiv2's XMP toolkit is not thread-safe and output pages are written from worker threads.
std::mutex& exiv2Mutex() {
  static std::mutex mutex;
  return mutex;
}

bool readFile(const QString& path, QByteArray* data) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  *data = file.readAll();
  return !data->isEmpty();
}

// Exiv2 0.27 returns Image::AutoPtr, 0.28 Image::UniquePtr.
auto openFromMemory(const QByteArray& data) {
  return Exiv2::ImageFactory::open(reinterpret_cast<const Exiv2::byte*>(data.constData()), data.size());
}

/** IFD0 tags that describe the pixel data or point to other structures. */
bool isStructuralImageTag(const uint16_t tag) {
  switch (tag) {
    case 0x00fe:  // NewSubfileType
    case 0x00ff:  // SubfileType
    case 0x0100:  // ImageWidth
    case 0x0101:  // ImageLength
    case 0x0102:  // BitsPerSample
    case 0x0103:  // Compression
    case 0x0106:  // PhotometricInterpretation
    case 0x0107:  // Thresholding
    case 0x0108:  // CellWidth
    case 0x0109:  // CellLength
    case 0x010a:  // FillOrder
    case 0x0111:  // StripOffsets
    case 0x0112:  // Orientation
    case 0x0115:  // SamplesPerPixel
    case 0x0116:  // RowsPerStrip
    case 0x0117:  // StripByteCounts
    case 0x011a:  // XResolution
    case 0x011b:  // YResolution
    case 0x011c:  // PlanarConfiguration
    case 0x0128:  // ResolutionUnit
    case 0x013d:  // Predictor
    case 0x0140:  // ColorMap
    case 0x0142:  // TileWidth
    case 0x0143:  // TileLength
    case 0x0144:  // TileOffsets
    case 0x0145:  // TileByteCounts
    case 0x014a:  // SubIFDs
    case 0x0152:  // ExtraSamples
    case 0x0153:  // SampleFormat
    case 0x0200:  // JPEGProc
    case 0x0201:  // JPEGInterchangeFormat
    case 0x0202:  // JPEGInterchangeFormatLength
    case 0x0211:  // YCbCrCoefficients
    case 0x0212:  // YCbCrSubSampling
    case 0x0213:  // YCbCrPositioning
    case 0x02bc:  // XMLPacket, copied as XMP instead
    case 0x828d:  // CFARepeatPatternDim
    case 0x828e:  // CFAPattern
    case 0x83bb:  // IPTCNAA, copied as IPTC instead
    case 0x8649:  // Photoshop ImageResources
    case 0x8769:  // ExifTag
    case 0x8773:  // InterColorProfile
    case 0x8825:  // GPSTag
      return true;
    default:
      break;
  }
  // DNG raw-processing tags (DNGVersion ... ) describe the raw data, not the output.
  return (tag >= 0xc612) && (tag <= 0xc7ff);
}

bool shouldCopyExif(const Exiv2::Exifdatum& datum) {
  const std::string group = datum.groupName();
  const uint16_t tag = datum.tag();
  if (group == "Image") {
    return !isStructuralImageTag(tag);
  }
  if (group == "Photo") {
    switch (tag) {
      case 0xa001:  // ColorSpace may no longer match the output ICC profile
      case 0xa002:  // PixelXDimension
      case 0xa003:  // PixelYDimension
      case 0xa005:  // InteroperabilityTag
        return false;
      default:
        return true;
    }
  }
  if ((group == "GPSInfo") || (group == "Iop")) {
    return true;
  }
  // Thumbnail, SubImage*, Image2..., i.e. other images of the source file, are dropped.
  return Exiv2::ExifTags::isMakerGroup(group);
}

bool startsWith(const std::string& str, const char* prefix) {
  return str.rfind(prefix, 0) == 0;
}

bool shouldCopyXmp(const Exiv2::Xmpdatum& datum) {
  static const char* const droppedPrefixes[] = {
      "Xmp.tiff.ImageWidth",
      "Xmp.tiff.ImageLength",
      "Xmp.tiff.BitsPerSample",
      "Xmp.tiff.Compression",
      "Xmp.tiff.PhotometricInterpretation",
      "Xmp.tiff.Orientation",
      "Xmp.tiff.SamplesPerPixel",
      "Xmp.tiff.PlanarConfiguration",
      "Xmp.tiff.YCbCr",
      "Xmp.tiff.XResolution",
      "Xmp.tiff.YResolution",
      "Xmp.tiff.ResolutionUnit",
      "Xmp.exif.PixelXDimension",
      "Xmp.exif.PixelYDimension",
      "Xmp.exif.ColorSpace",
      "Xmp.photoshop.ICCProfile",
      "Xmp.xmp.Thumbnails",
      "Xmp.xmpMM.InstanceID",
      // Camera Raw settings would be applied again by raw-aware software.
      "Xmp.crs.",
      "Xmp.crss.",
      // Descriptions of data embedded in the source file (gain maps, motion photo video,
      // depth maps, extended XMP) that the output does not contain.
      "Xmp.Container.",
      "Xmp.GContainer.",
      "Xmp.hdrgm.",
      "Xmp.GCamera.",
      "Xmp.GDepth.",
      "Xmp.GImage.",
      "Xmp.GAudio.",
      "Xmp.xmpNote.HasExtendedXMP",
  };
  const std::string key = datum.key();
  for (const char* prefix : droppedPrefixes) {
    if (startsWith(key, prefix)) {
      return false;
    }
  }
  return true;
}

/** Returns false if the source had nothing worth copying and \p targetData was left untouched. */
bool copyMetadataLocked(const QByteArray& sourceData, QByteArray* targetData) {
  static const bool logLevelSet = [] {
    // Damaged maker notes and the like are reported per file, not worth a console message each.
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);
    return true;
  }();
  Q_UNUSED(logLevelSet);

  auto source = openFromMemory(sourceData);
  source->readMetadata();

  auto target = openFromMemory(*targetData);
  target->readMetadata();

  bool copiedExif = false;
  Exiv2::ExifData& targetExif = target->exifData();
  for (const Exiv2::Exifdatum& datum : source->exifData()) {
    if (!shouldCopyExif(datum)) {
      continue;
    }
    const Exiv2::ExifKey key(datum.key());
    if (targetExif.findKey(key) != targetExif.end()) {
      continue;  // Keep what the output writer produced.
    }
    targetExif.add(datum);
    copiedExif = true;
  }
  if (copiedExif) {
    targetExif["Exif.Image.Orientation"] = uint16_t(1);
    const auto width = static_cast<uint32_t>(target->pixelWidth());
    const auto height = static_cast<uint32_t>(target->pixelHeight());
    if ((width > 0) && (height > 0)) {
      targetExif["Exif.Photo.PixelXDimension"] = width;
      targetExif["Exif.Photo.PixelYDimension"] = height;
    }
  }

  Exiv2::XmpData& targetXmp = target->xmpData();
  bool copiedXmp = false;
  for (const Exiv2::Xmpdatum& datum : source->xmpData()) {
    if (!shouldCopyXmp(datum)) {
      continue;
    }
    if (targetXmp.findKey(Exiv2::XmpKey(datum.key())) != targetXmp.end()) {
      continue;
    }
    targetXmp.add(datum);
    copiedXmp = true;
  }
  if (copiedXmp && source->xmpData().findKey(Exiv2::XmpKey("Xmp.tiff.Orientation")) != source->xmpData().end()) {
    targetXmp["Xmp.tiff.Orientation"] = std::string("1");
  }

  Exiv2::IptcData& targetIptc = target->iptcData();
  for (const Exiv2::Iptcdatum& datum : source->iptcData()) {
    targetIptc.add(datum);
  }

  if (!copiedExif && !copiedXmp && source->iptcData().empty()) {
    return false;
  }

  target->writeMetadata();

  Exiv2::BasicIo& io = target->io();
  io.open();
  const auto size = io.size();
  const Exiv2::byte* bytes = io.mmap();
  *targetData = QByteArray(reinterpret_cast<const char*>(bytes), static_cast<qsizetype>(size));
  io.munmap();
  io.close();
  return true;
}
}  // namespace

bool isCopySupported() {
  return true;
}

bool copyMetadata(const QString& sourcePath, const QString& targetPath, QString* errorMessage) {
  QByteArray sourceData;
  if (!readFile(sourcePath, &sourceData)) {
    return true;  // Nothing to copy from.
  }
  QByteArray targetData;
  if (!readFile(targetPath, &targetData)) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("cannot read %1").arg(targetPath);
    }
    return false;
  }

  bool changed = false;
  try {
    std::lock_guard<std::mutex> lock(exiv2Mutex());
    changed = copyMetadataLocked(sourceData, &targetData);
  } catch (const std::exception& e) {
    if (errorMessage) {
      *errorMessage = QString::fromLocal8Bit(e.what());
    }
    return false;
  }

  if (!changed) {
    return true;
  }

  QSaveFile file(targetPath);
  if (!file.open(QIODevice::WriteOnly) || (file.write(targetData) != targetData.size()) || !file.commit()) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("cannot write %1").arg(targetPath);
    }
    return false;
  }
  return true;
}

#endif  // SCANTAILOR_HAVE_EXIV2
}  // namespace image_metadata
