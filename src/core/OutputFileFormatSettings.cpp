// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputFileFormatSettings.h"

namespace {
QString normalizeToken(QString value) {
  value = value.trimmed().toLower();
  value.remove(QLatin1Char('_'));
  value.remove(QLatin1Char('-'));
  value.remove(QLatin1Char(' '));
  return value;
}
}  // namespace

QString outputColorProfileModeToString(const OutputColorProfileMode mode) {
  switch (mode) {
    case OutputColorProfileMode::Source:
      return QStringLiteral("source");
    case OutputColorProfileMode::SRgb:
      return QStringLiteral("srgb");
    case OutputColorProfileMode::AdobeRgb:
      return QStringLiteral("adobergb");
    case OutputColorProfileMode::EciRgbV2:
      return QStringLiteral("ecirgbv2");
    case OutputColorProfileMode::Custom:
      return QStringLiteral("custom");
  }

  return QStringLiteral("source");
}

bool outputColorProfileModeFromString(const QString& str, OutputColorProfileMode* mode) {
  if (!mode) {
    return false;
  }

  const QString token = normalizeToken(str);
  if (token == QStringLiteral("source") || token == QStringLiteral("embedded")) {
    *mode = OutputColorProfileMode::Source;
    return true;
  }
  if (token == QStringLiteral("srgb")) {
    *mode = OutputColorProfileMode::SRgb;
    return true;
  }
  if (token == QStringLiteral("adobergb") || token == QStringLiteral("adobe1998")) {
    *mode = OutputColorProfileMode::AdobeRgb;
    return true;
  }
  if (token == QStringLiteral("ecirgbv2") || token == QStringLiteral("ecirgb") || token == QStringLiteral("eci")) {
    *mode = OutputColorProfileMode::EciRgbV2;
    return true;
  }
  if (token == QStringLiteral("custom")) {
    *mode = OutputColorProfileMode::Custom;
    return true;
  }

  return false;
}

QString outputRenderingIntentToString(const OutputRenderingIntent intent) {
  switch (intent) {
    case OutputRenderingIntent::Perceptual:
      return QStringLiteral("perceptual");
    case OutputRenderingIntent::RelativeColorimetric:
      return QStringLiteral("relative-colorimetric");
    case OutputRenderingIntent::Saturation:
      return QStringLiteral("saturation");
    case OutputRenderingIntent::AbsoluteColorimetric:
      return QStringLiteral("absolute-colorimetric");
  }

  return QStringLiteral("relative-colorimetric");
}

bool outputRenderingIntentFromString(const QString& str, OutputRenderingIntent* intent) {
  if (!intent) {
    return false;
  }

  const QString token = normalizeToken(str);
  if (token == QStringLiteral("perceptual")) {
    *intent = OutputRenderingIntent::Perceptual;
    return true;
  }
  if (token == QStringLiteral("relativecolorimetric") || token == QStringLiteral("relative")) {
    *intent = OutputRenderingIntent::RelativeColorimetric;
    return true;
  }
  if (token == QStringLiteral("saturation")) {
    *intent = OutputRenderingIntent::Saturation;
    return true;
  }
  if (token == QStringLiteral("absolutecolorimetric") || token == QStringLiteral("absolute")) {
    *intent = OutputRenderingIntent::AbsoluteColorimetric;
    return true;
  }

  return false;
}

OutputFileFormatSettings& OutputFileFormatSettings::getInstance() {
  static OutputFileFormatSettings settings;
  return settings;
}

OutputFileFormat OutputFileFormatSettings::format() const {
  return m_format;
}

void OutputFileFormatSettings::setFormat(const OutputFileFormat format) {
  m_format = format;
}

OutputColorProfileMode OutputFileFormatSettings::colorProfileMode() const {
  return m_colorProfileMode;
}

void OutputFileFormatSettings::setColorProfileMode(const OutputColorProfileMode mode) {
  m_colorProfileMode = mode;
}

OutputRenderingIntent OutputFileFormatSettings::renderingIntent() const {
  return m_renderingIntent;
}

void OutputFileFormatSettings::setRenderingIntent(const OutputRenderingIntent intent) {
  m_renderingIntent = intent;
}

QString OutputFileFormatSettings::customIccProfilePath() const {
  return m_customIccProfilePath;
}

void OutputFileFormatSettings::setCustomIccProfilePath(const QString& path) {
  m_customIccProfilePath = path;
}

void OutputFileFormatSettings::resetToDefault() {
  m_format = OutputFileFormat::Tiff;
  m_colorProfileMode = OutputColorProfileMode::Source;
  m_renderingIntent = OutputRenderingIntent::RelativeColorimetric;
  m_customIccProfilePath.clear();
}
