// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_RAWIMAGELOADER_H_
#define SCANTAILOR_CORE_RAWIMAGELOADER_H_

#include <QImage>
#include <QString>
#include <QStringList>

struct RawLoadParams {
  enum class Demosaic {
    Linear = 0,
    VNG = 1,
    PPG = 2,
    AHD = 3,
    DCB = 4,
    DHT = 11,
  };

  enum class WhiteBalance {
    Camera,
    Auto,
    Daylight,
    Manual,
  };

  Demosaic demosaic = Demosaic::AHD;
  WhiteBalance whiteBalance = WhiteBalance::Camera;
  float wbMult[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  bool useCameraMatrix = true;
  float exposureShift = 0.0f;
  bool halfSize = false;
};

class RawImageLoader {
 public:
  static QImage load(const QString& path, const RawLoadParams& params = RawLoadParams());

  static QImage loadThumbnail(const QString& path);

  static bool isRawFile(const QString& path);

  static QStringList supportedExtensions();
};

#endif  // SCANTAILOR_CORE_RAWIMAGELOADER_H_
