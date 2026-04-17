// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_RAWSETTINGS_H_
#define SCANTAILOR_CORE_RAWSETTINGS_H_

#include <QString>

#include "RawImageLoader.h"

class RawSettings {
 public:
  static RawSettings& getInstance();

  const RawLoadParams& getParams() const;

  void setParams(const RawLoadParams& params);

  void resetToDefaults();

 private:
  RawSettings() = default;

  RawLoadParams m_params;
};

bool operator==(const RawLoadParams& lhs, const RawLoadParams& rhs);

bool operator!=(const RawLoadParams& lhs, const RawLoadParams& rhs);

QString rawDemosaicToString(RawLoadParams::Demosaic demosaic);

bool rawDemosaicFromString(const QString& str, RawLoadParams::Demosaic* demosaic);

QString rawWhiteBalanceToString(RawLoadParams::WhiteBalance whiteBalance);

bool rawWhiteBalanceFromString(const QString& str, RawLoadParams::WhiteBalance* whiteBalance);

#endif  // SCANTAILOR_CORE_RAWSETTINGS_H_
