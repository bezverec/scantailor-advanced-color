// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEFILEWRITER_H_
#define SCANTAILOR_CORE_IMAGEFILEWRITER_H_

#include <QImage>
#include <QString>

#include "OutputFileFormat.h"
#include "OutputFileFormatSettings.h"

class ImageFileWriter {
 public:
  static bool writeImage(const QString& filePath, const QImage& image);

  static bool writeImage(const QString& filePath, const QImage& image, OutputFileFormat format);

  /**
   * Writes the image in the current output format and, if enabled in
   * OutputFileFormatSettings, copies EXIF / XMP / IPTC metadata from \p metadataSourcePath.
   * Failing to copy metadata does not fail the write.
   */
  static bool writeImage(const QString& filePath, const QImage& image, const QString& metadataSourcePath);

  /**
   * Converts colour / grayscale images to 8 or 16 bits per channel, keeping resolution,
   * text keys and colour space. Black and white (1-bit) and palette images are returned
   * as they are, as is everything for OutputBitDepth::Source.
   */
  static QImage convertToBitDepth(const QImage& image, OutputBitDepth bitDepth);
};

#endif  // SCANTAILOR_CORE_IMAGEFILEWRITER_H_
