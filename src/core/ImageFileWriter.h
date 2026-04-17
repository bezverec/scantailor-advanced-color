// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEFILEWRITER_H_
#define SCANTAILOR_CORE_IMAGEFILEWRITER_H_

#include <QImage>
#include <QString>

#include "OutputFileFormat.h"

class ImageFileWriter {
 public:
  static bool writeImage(const QString& filePath, const QImage& image);

  static bool writeImage(const QString& filePath, const QImage& image, OutputFileFormat format);
};

#endif  // SCANTAILOR_CORE_IMAGEFILEWRITER_H_
