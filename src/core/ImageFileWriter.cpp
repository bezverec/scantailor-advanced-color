// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageFileWriter.h"

#include <QColorSpace>
#include <QImageWriter>

#include "ColorProfileUtils.h"
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

bool ImageFileWriter::writeImage(const QString& filePath, const QImage& image, const OutputFileFormat format) {
  if (image.isNull()) {
    return false;
  }

  QImage writeImage(color_profile::prepareForOutput(image, OutputFileFormatSettings::getInstance().colorProfileMode(),
                                                    OutputFileFormatSettings::getInstance().renderingIntent(),
                                                    OutputFileFormatSettings::getInstance().customIccProfilePath()));

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
