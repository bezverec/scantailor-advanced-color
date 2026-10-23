// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageFileWriter.h"

#include <QColorSpace>
#include <QImageWriter>

#include <QDebug>

#include "ColorProfileUtils.h"
#include "ImageMetadataCopier.h"
#include "OutputFileFormatSettings.h"
#include "TiffWriter.h"

namespace {
QImage flattenForJpeg(const QImage& image) {
  QImage converted = image.convertToFormat(QImage::Format_RGB32);
  if (image.colorSpace().isValid()) {
    converted.setColorSpace(image.colorSpace());
  }
  return converted;
}
}  // namespace

bool ImageFileWriter::writeImage(const QString& filePath, const QImage& image) {
  return writeImage(filePath, image, OutputFileFormatSettings::getInstance().format());
}

bool ImageFileWriter::writeImage(const QString& filePath, const QImage& image, const QString& metadataSourcePath) {
  if (!writeImage(filePath, image)) {
    return false;
  }
  if (OutputFileFormatSettings::getInstance().preserveMetadata() && image_metadata::isCopySupported()
      && !metadataSourcePath.isEmpty()) {
    QString error;
    if (!image_metadata::copyMetadata(metadataSourcePath, filePath, &error)) {
      qWarning().noquote() << "Could not copy metadata from" << metadataSourcePath << "to" << filePath << ":" << error;
    }
  }
  return true;
}

QImage ImageFileWriter::convertToBitDepth(const QImage& image, const OutputBitDepth bitDepth) {
  if (image.isNull() || (bitDepth == OutputBitDepth::Source)) {
    return image;
  }

  switch (image.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      return image;
    case QImage::Format_Indexed8:
      if ((bitDepth == OutputBitDepth::Bits16) && image.isGrayscale()) {
        return image.convertToFormat(QImage::Format_Grayscale16);
      }
      return image;
    case QImage::Format_Grayscale8:
      return (bitDepth == OutputBitDepth::Bits16) ? image.convertToFormat(QImage::Format_Grayscale16) : image;
    case QImage::Format_Grayscale16:
      return (bitDepth == OutputBitDepth::Bits8) ? image.convertToFormat(QImage::Format_Grayscale8) : image;
    default:
      break;
  }

  const bool is16Bit = (image.depth() == 64);
  if ((bitDepth == OutputBitDepth::Bits8) && is16Bit) {
    return image.convertToFormat(image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
  }
  if ((bitDepth == OutputBitDepth::Bits16) && !is16Bit) {
    return image.convertToFormat(image.hasAlphaChannel() ? QImage::Format_RGBA64 : QImage::Format_RGBX64);
  }
  return image;
}

bool ImageFileWriter::writeImage(const QString& filePath, const QImage& image, const OutputFileFormat format) {
  if (image.isNull()) {
    return false;
  }

  // Widen before and narrow after the colour conversion, so it always runs at the higher precision.
  const OutputBitDepth bitDepth
      = (format == OutputFileFormat::Jpeg) ? OutputBitDepth::Source : OutputFileFormatSettings::getInstance().bitDepth();
  QImage writeImage(color_profile::prepareForOutput(
      (bitDepth == OutputBitDepth::Bits16) ? convertToBitDepth(image, bitDepth) : image,
      OutputFileFormatSettings::getInstance().colorProfileMode(), OutputFileFormatSettings::getInstance().renderingIntent(),
      OutputFileFormatSettings::getInstance().customIccProfilePath()));
  if (bitDepth == OutputBitDepth::Bits8) {
    writeImage = convertToBitDepth(writeImage, bitDepth);
  }

  if (format == OutputFileFormat::Tiff) {
    return TiffWriter::writeImage(filePath, writeImage);
  }

  if (format == OutputFileFormat::Jpeg) {
    writeImage = flattenForJpeg(writeImage);
  }

  QImageWriter writer(filePath, outputFileFormatQtWriterFormat(format));
  if (format == OutputFileFormat::Jpeg) {
    writer.setQuality(90);
  } else if (format == OutputFileFormat::Png) {
    writer.setCompression(1);
  }

  return writer.write(writeImage);
}
