// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorProfileUtils.h"

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QColorSpace>
#endif

#include <QImage>

#ifdef SCANTAILOR_HAVE_LCMS2
#include <lcms2.h>
#endif

namespace color_profile {
namespace {

bool isGrayLike(const QImage& image) {
  return image.isGrayscale() || image.format() == QImage::Format_Mono || image.format() == QImage::Format_MonoLSB;
}

#ifdef SCANTAILOR_HAVE_LCMS2
QByteArray saveProfileToByteArray(cmsHPROFILE profile) {
  if (!profile) {
    return QByteArray();
  }

  cmsUInt32Number profileSize = 0;
  if (!cmsSaveProfileToMem(profile, nullptr, &profileSize) || profileSize == 0) {
    return QByteArray();
  }

  QByteArray iccProfile(int(profileSize), Qt::Uninitialized);
  if (!cmsSaveProfileToMem(profile, iccProfile.data(), &profileSize)) {
    return QByteArray();
  }

  iccProfile.resize(int(profileSize));
  return iccProfile;
}

QByteArray createGrayProfile() {
  cmsToneCurve* toneCurve = cmsBuildGamma(nullptr, 2.2);
  if (!toneCurve) {
    return QByteArray();
  }

  const cmsCIExyY whitePoint = {0.3457, 0.3585, 1.0};
  cmsHPROFILE profile = cmsCreateGrayProfile(&whitePoint, toneCurve);
  cmsFreeToneCurve(toneCurve);

  const QByteArray iccProfile(saveProfileToByteArray(profile));
  if (profile) {
    cmsCloseProfile(profile);
  }
  return iccProfile;
}

QByteArray createSrgbProfile() {
  cmsHPROFILE profile = cmsCreate_sRGBProfile();
  const QByteArray iccProfile(saveProfileToByteArray(profile));
  if (profile) {
    cmsCloseProfile(profile);
  }
  return iccProfile;
}
#endif

}  // namespace

QByteArray defaultIccProfileForImage(const QImage& image) {
#ifdef SCANTAILOR_HAVE_LCMS2
  return isGrayLike(image) ? createGrayProfile() : createSrgbProfile();
#else
  Q_UNUSED(image);
  return QByteArray();
#endif
}

QByteArray effectiveIccProfileForImage(const QImage& image) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  if (image.colorSpace().isValid()) {
    const QByteArray iccProfile(image.colorSpace().iccProfile());
    if (!iccProfile.isEmpty()) {
      return iccProfile;
    }
  }
#endif
  return defaultIccProfileForImage(image);
}

bool hasColorProfile(const QImage& image) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return image.colorSpace().isValid();
#else
  Q_UNUSED(image);
  return false;
#endif
}

void applyIccProfile(QImage& image, const QByteArray& iccProfile) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  if (iccProfile.isEmpty()) {
    return;
  }

  const QColorSpace colorSpace(QColorSpace::fromIccProfile(iccProfile));
  if (colorSpace.isValid()) {
    image.setColorSpace(colorSpace);
  }
#else
  Q_UNUSED(image);
  Q_UNUSED(iccProfile);
#endif
}

void applyDefaultColorProfile(QImage& image) {
  applyIccProfile(image, defaultIccProfileForImage(image));
}

void copyColorProfile(const QImage& source, QImage& target) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  if (source.colorSpace().isValid()) {
    target.setColorSpace(source.colorSpace());
    return;
  }
#else
  Q_UNUSED(source);
#endif
  applyDefaultColorProfile(target);
}

}  // namespace color_profile
