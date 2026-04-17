// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_COLORPROFILEUTILS_H_
#define SCANTAILOR_CORE_COLORPROFILEUTILS_H_

#include <QByteArray>
#include <QString>

#include "OutputFileFormatSettings.h"

class QImage;

namespace color_profile {

QByteArray defaultIccProfileForImage(const QImage& image);

QByteArray effectiveIccProfileForImage(const QImage& image);

bool hasColorProfile(const QImage& image);

void applyIccProfile(QImage& image, const QByteArray& iccProfile);

void applyDefaultColorProfile(QImage& image);

void copyColorProfile(const QImage& source, QImage& target);

QByteArray readIccProfileFromFile(const QString& path);

QByteArray builtInIccProfile(OutputColorProfileMode mode, const QImage& image);

QImage prepareForOutput(const QImage& image,
                        OutputColorProfileMode profileMode,
                        OutputRenderingIntent renderingIntent,
                        const QString& customProfilePath);

}  // namespace color_profile

#endif  // ifndef SCANTAILOR_CORE_COLORPROFILEUTILS_H_
