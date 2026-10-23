// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_OUTPUTFILEFORMATSETTINGS_H_
#define SCANTAILOR_CORE_OUTPUTFILEFORMATSETTINGS_H_

#include <QString>

#include "OutputFileFormat.h"

enum class OutputColorProfileMode { Source, SRgb, AdobeRgb, EciRgbV2, Custom };

enum class OutputRenderingIntent { Perceptual, RelativeColorimetric, Saturation, AbsoluteColorimetric };

QString outputColorProfileModeToString(OutputColorProfileMode mode);

bool outputColorProfileModeFromString(const QString& str, OutputColorProfileMode* mode);

QString outputRenderingIntentToString(OutputRenderingIntent intent);

bool outputRenderingIntentFromString(const QString& str, OutputRenderingIntent* intent);

class OutputFileFormatSettings {
 public:
  static OutputFileFormatSettings& getInstance();

  OutputFileFormat format() const;

  void setFormat(OutputFileFormat format);

  OutputColorProfileMode colorProfileMode() const;

  void setColorProfileMode(OutputColorProfileMode mode);

  OutputRenderingIntent renderingIntent() const;

  void setRenderingIntent(OutputRenderingIntent intent);

  QString customIccProfilePath() const;

  void setCustomIccProfilePath(const QString& path);

  /** Copy EXIF / XMP / IPTC metadata from the source image to the output files. */
  bool preserveMetadata() const;

  void setPreserveMetadata(bool preserve);

  void resetToDefault();

 private:
  OutputFileFormatSettings() = default;

  OutputFileFormat m_format = OutputFileFormat::Tiff;
  OutputColorProfileMode m_colorProfileMode = OutputColorProfileMode::Source;
  OutputRenderingIntent m_renderingIntent = OutputRenderingIntent::RelativeColorimetric;
  QString m_customIccProfilePath;
  bool m_preserveMetadata = true;
};

#endif  // SCANTAILOR_CORE_OUTPUTFILEFORMATSETTINGS_H_
