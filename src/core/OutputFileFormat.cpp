// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputFileFormat.h"

QString outputFileFormatToString(const OutputFileFormat format) {
  switch (format) {
    case OutputFileFormat::Tiff:
      return QStringLiteral("tiff");
    case OutputFileFormat::Png:
      return QStringLiteral("png");
    case OutputFileFormat::Jpeg:
      return QStringLiteral("jpeg");
  }

  return QStringLiteral("tiff");
}

bool outputFileFormatFromString(const QString& str, OutputFileFormat* const format) {
  if (format == nullptr) {
    return false;
  }

  if (str == QStringLiteral("tif") || str == QStringLiteral("tiff")) {
    *format = OutputFileFormat::Tiff;
  } else if (str == QStringLiteral("png")) {
    *format = OutputFileFormat::Png;
  } else if (str == QStringLiteral("jpg") || str == QStringLiteral("jpeg")) {
    *format = OutputFileFormat::Jpeg;
  } else {
    return false;
  }

  return true;
}

QString outputFileFormatExtension(const OutputFileFormat format) {
  switch (format) {
    case OutputFileFormat::Tiff:
      return QStringLiteral("tif");
    case OutputFileFormat::Png:
      return QStringLiteral("png");
    case OutputFileFormat::Jpeg:
      return QStringLiteral("jpg");
  }

  return QStringLiteral("tif");
}

QByteArray outputFileFormatQtWriterFormat(const OutputFileFormat format) {
  switch (format) {
    case OutputFileFormat::Tiff:
      return QByteArrayLiteral("tiff");
    case OutputFileFormat::Png:
      return QByteArrayLiteral("png");
    case OutputFileFormat::Jpeg:
      return QByteArrayLiteral("jpeg");
  }

  return QByteArrayLiteral("tiff");
}

bool outputFileFormatIsLossy(const OutputFileFormat format) {
  return format == OutputFileFormat::Jpeg;
}

std::vector<OutputFileFormat> allOutputFileFormats() {
  return {OutputFileFormat::Tiff, OutputFileFormat::Png, OutputFileFormat::Jpeg};
}
