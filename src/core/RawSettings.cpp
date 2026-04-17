// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RawSettings.h"

#include <cmath>

namespace {
template <typename T>
bool compareFloatArray(const T* lhs, const T* rhs, const int size) {
  for (int i = 0; i < size; ++i) {
    if (std::fabs(lhs[i] - rhs[i]) > 0.000001f) {
      return false;
    }
  }
  return true;
}
}  // namespace

RawSettings& RawSettings::getInstance() {
  static RawSettings settings;
  return settings;
}

const RawLoadParams& RawSettings::getParams() const {
  return m_params;
}

void RawSettings::setParams(const RawLoadParams& params) {
  m_params = params;
}

void RawSettings::resetToDefaults() {
  m_params = RawLoadParams();
}

bool operator==(const RawLoadParams& lhs, const RawLoadParams& rhs) {
  return lhs.demosaic == rhs.demosaic
         && lhs.whiteBalance == rhs.whiteBalance
         && compareFloatArray(lhs.wbMult, rhs.wbMult, 4)
         && lhs.useCameraMatrix == rhs.useCameraMatrix
         && std::fabs(lhs.exposureShift - rhs.exposureShift) <= 0.000001f
         && lhs.halfSize == rhs.halfSize;
}

bool operator!=(const RawLoadParams& lhs, const RawLoadParams& rhs) {
  return !(lhs == rhs);
}

QString rawDemosaicToString(const RawLoadParams::Demosaic demosaic) {
  switch (demosaic) {
    case RawLoadParams::Demosaic::Linear:
      return QStringLiteral("linear");
    case RawLoadParams::Demosaic::VNG:
      return QStringLiteral("vng");
    case RawLoadParams::Demosaic::PPG:
      return QStringLiteral("ppg");
    case RawLoadParams::Demosaic::AHD:
      return QStringLiteral("ahd");
    case RawLoadParams::Demosaic::DCB:
      return QStringLiteral("dcb");
    case RawLoadParams::Demosaic::DHT:
      return QStringLiteral("dht");
  }

  return QStringLiteral("ahd");
}

bool rawDemosaicFromString(const QString& str, RawLoadParams::Demosaic* const demosaic) {
  if (demosaic == nullptr) {
    return false;
  }

  if (str == QStringLiteral("linear")) {
    *demosaic = RawLoadParams::Demosaic::Linear;
  } else if (str == QStringLiteral("vng")) {
    *demosaic = RawLoadParams::Demosaic::VNG;
  } else if (str == QStringLiteral("ppg")) {
    *demosaic = RawLoadParams::Demosaic::PPG;
  } else if (str == QStringLiteral("ahd")) {
    *demosaic = RawLoadParams::Demosaic::AHD;
  } else if (str == QStringLiteral("dcb")) {
    *demosaic = RawLoadParams::Demosaic::DCB;
  } else if (str == QStringLiteral("dht")) {
    *demosaic = RawLoadParams::Demosaic::DHT;
  } else {
    return false;
  }

  return true;
}

QString rawWhiteBalanceToString(const RawLoadParams::WhiteBalance whiteBalance) {
  switch (whiteBalance) {
    case RawLoadParams::WhiteBalance::Camera:
      return QStringLiteral("camera");
    case RawLoadParams::WhiteBalance::Auto:
      return QStringLiteral("auto");
    case RawLoadParams::WhiteBalance::Daylight:
      return QStringLiteral("daylight");
    case RawLoadParams::WhiteBalance::Manual:
      return QStringLiteral("manual");
  }

  return QStringLiteral("camera");
}

bool rawWhiteBalanceFromString(const QString& str, RawLoadParams::WhiteBalance* const whiteBalance) {
  if (whiteBalance == nullptr) {
    return false;
  }

  if (str == QStringLiteral("camera")) {
    *whiteBalance = RawLoadParams::WhiteBalance::Camera;
  } else if (str == QStringLiteral("auto")) {
    *whiteBalance = RawLoadParams::WhiteBalance::Auto;
  } else if (str == QStringLiteral("daylight")) {
    *whiteBalance = RawLoadParams::WhiteBalance::Daylight;
  } else if (str == QStringLiteral("manual")) {
    *whiteBalance = RawLoadParams::WhiteBalance::Manual;
  } else {
    return false;
  }

  return true;
}
