// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorProfileUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QColorSpace>
#endif

#ifdef SCANTAILOR_HAVE_LCMS2
#include <lcms2.h>
#endif

namespace color_profile {
namespace {

bool isGrayLike(const QImage& image) {
  return image.isGrayscale() || image.format() == QImage::Format_Mono || image.format() == QImage::Format_MonoLSB;
}

QString bundledProfileFileName(const OutputColorProfileMode mode) {
  switch (mode) {
    case OutputColorProfileMode::SRgb:
      return QStringLiteral("sRGB_v4_ICC_preference.icc");
    case OutputColorProfileMode::AdobeRgb:
      return QStringLiteral("AdobeRGB1998.icc");
    case OutputColorProfileMode::EciRgbV2:
      return QStringLiteral("eciRGB_v2_ICCv4.icc");
    case OutputColorProfileMode::Source:
    case OutputColorProfileMode::Custom:
      break;
  }

  return QString();
}

QString bundledProfilePath(const QString& fileName) {
  if (fileName.isEmpty()) {
    return QString();
  }

  QStringList candidatePaths;
  const QString appDirPath = QCoreApplication::applicationDirPath();
  if (!appDirPath.isEmpty()) {
    const QDir appDir(appDirPath);
    candidatePaths << appDir.filePath(QStringLiteral("icc/%1").arg(fileName))
                   << appDir.filePath(fileName)
                   << appDir.absoluteFilePath(QStringLiteral("../icc/%1").arg(fileName));
  }

  const QDir currentDir(QDir::currentPath());
  candidatePaths << currentDir.filePath(QStringLiteral("icc/%1").arg(fileName))
                 << currentDir.filePath(fileName);

  for (const QString& candidatePath : candidatePaths) {
    const QFileInfo info(candidatePath);
    if (info.exists() && info.isFile()) {
      return info.absoluteFilePath();
    }
  }

  return QString();
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

QByteArray createAdobeRgbProfile() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const QColorSpace colorSpace(QColorSpace::AdobeRgb);
  if (colorSpace.isValid()) {
    const QByteArray iccProfile(colorSpace.iccProfile());
    if (!iccProfile.isEmpty()) {
      return iccProfile;
    }
  }
#endif
  return QByteArray();
}

int lcmsIntent(const OutputRenderingIntent renderingIntent) {
  switch (renderingIntent) {
    case OutputRenderingIntent::Perceptual:
      return INTENT_PERCEPTUAL;
    case OutputRenderingIntent::RelativeColorimetric:
      return INTENT_RELATIVE_COLORIMETRIC;
    case OutputRenderingIntent::Saturation:
      return INTENT_SATURATION;
    case OutputRenderingIntent::AbsoluteColorimetric:
      return INTENT_ABSOLUTE_COLORIMETRIC;
  }

  return INTENT_RELATIVE_COLORIMETRIC;
}

QImage convertColorImageWithLcms(const QImage& image, const QByteArray& srcProfile, const QByteArray& dstProfile,
                                 const OutputRenderingIntent renderingIntent) {
  if (image.isNull() || srcProfile.isEmpty() || dstProfile.isEmpty() || srcProfile == dstProfile) {
    return image;
  }

  cmsHPROFILE src = cmsOpenProfileFromMem(srcProfile.constData(), cmsUInt32Number(srcProfile.size()));
  cmsHPROFILE dst = cmsOpenProfileFromMem(dstProfile.constData(), cmsUInt32Number(dstProfile.size()));
  if (!src || !dst) {
    if (src) {
      cmsCloseProfile(src);
    }
    if (dst) {
      cmsCloseProfile(dst);
    }
    return image;
  }

  QImage srcImage;
  QImage outImage;
  cmsUInt32Number pixelType = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  if (image.depth() > 8) {
    srcImage = image.convertToFormat(QImage::Format_RGBA64);
    outImage = QImage(srcImage.size(), QImage::Format_RGBA64);
    pixelType = TYPE_RGBA_16;
  } else
#endif
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    srcImage = image.convertToFormat(QImage::Format_RGBA8888);
    outImage = QImage(srcImage.size(), QImage::Format_RGBA8888);
    pixelType = TYPE_RGBA_8;
#else
    srcImage = image.convertToFormat(QImage::Format_ARGB32);
    outImage = QImage(srcImage.size(), QImage::Format_ARGB32);
    pixelType = TYPE_BGRA_8;
#endif
  }

  if (srcImage.isNull() || outImage.isNull()) {
    cmsCloseProfile(src);
    cmsCloseProfile(dst);
    return image;
  }

  cmsHTRANSFORM transform
      = cmsCreateTransform(src, pixelType, dst, pixelType, lcmsIntent(renderingIntent), cmsFLAGS_COPY_ALPHA);
  if (!transform) {
    cmsCloseProfile(src);
    cmsCloseProfile(dst);
    return image;
  }

  for (int y = 0; y < srcImage.height(); ++y) {
    cmsDoTransform(transform, srcImage.constScanLine(y), outImage.scanLine(y), cmsUInt32Number(srcImage.width()));
  }

  cmsDeleteTransform(transform);
  cmsCloseProfile(src);
  cmsCloseProfile(dst);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const QColorSpace dstColorSpace(QColorSpace::fromIccProfile(dstProfile));
  if (dstColorSpace.isValid()) {
    outImage.setColorSpace(dstColorSpace);
  }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  if (image.depth() > 8) {
    if (image.hasAlphaChannel()) {
      return outImage.convertToFormat(QImage::Format_RGBA64);
    }
    return outImage.convertToFormat(QImage::Format_RGBX64);
  }
#endif

  if (image.hasAlphaChannel()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    return outImage.convertToFormat(QImage::Format_RGBA8888);
#else
    return outImage.convertToFormat(QImage::Format_ARGB32);
#endif
  }

  return outImage.convertToFormat(QImage::Format_RGB32);
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

QByteArray readIccProfileFromFile(const QString& path) {
  if (path.isEmpty()) {
    return QByteArray();
  }

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QByteArray();
  }

  return file.readAll();
}

QByteArray builtInIccProfile(const OutputColorProfileMode mode, const QImage& image) {
  if (isGrayLike(image)) {
#ifdef SCANTAILOR_HAVE_LCMS2
    return createGrayProfile();
#else
    return defaultIccProfileForImage(image);
#endif
  }

  const QByteArray bundledProfile(readIccProfileFromFile(bundledProfilePath(bundledProfileFileName(mode))));
  if (!bundledProfile.isEmpty()) {
    return bundledProfile;
  }

  switch (mode) {
    case OutputColorProfileMode::SRgb:
#ifdef SCANTAILOR_HAVE_LCMS2
      return createSrgbProfile();
#else
      return defaultIccProfileForImage(image);
#endif
    case OutputColorProfileMode::AdobeRgb:
#ifdef SCANTAILOR_HAVE_LCMS2
      return createAdobeRgbProfile();
#else
      return defaultIccProfileForImage(image);
#endif
    case OutputColorProfileMode::EciRgbV2:
      return defaultIccProfileForImage(image);
    case OutputColorProfileMode::Source:
    case OutputColorProfileMode::Custom:
      break;
  }

  return QByteArray();
}

QImage prepareForOutput(const QImage& image,
                        const OutputColorProfileMode profileMode,
                        const OutputRenderingIntent renderingIntent,
                        const QString& customProfilePath) {
  if (image.isNull()) {
    return image;
  }

  QByteArray targetProfile;
  switch (profileMode) {
    case OutputColorProfileMode::Source:
      targetProfile = effectiveIccProfileForImage(image);
      break;
    case OutputColorProfileMode::SRgb:
    case OutputColorProfileMode::AdobeRgb:
    case OutputColorProfileMode::EciRgbV2:
      targetProfile = builtInIccProfile(profileMode, image);
      break;
    case OutputColorProfileMode::Custom:
      targetProfile = readIccProfileFromFile(customProfilePath);
      break;
  }

  if (targetProfile.isEmpty()) {
    QImage fallback(image);
    if (!hasColorProfile(fallback)) {
      applyDefaultColorProfile(fallback);
    }
    return fallback;
  }

  if (profileMode == OutputColorProfileMode::Source || isGrayLike(image)) {
    QImage tagged(image);
    applyIccProfile(tagged, targetProfile);
    if (!hasColorProfile(tagged)) {
      applyDefaultColorProfile(tagged);
    }
    return tagged;
  }

#ifdef SCANTAILOR_HAVE_LCMS2
  const QByteArray sourceProfile(effectiveIccProfileForImage(image));
  QImage converted(convertColorImageWithLcms(image, sourceProfile, targetProfile, renderingIntent));
  if (!hasColorProfile(converted)) {
    applyIccProfile(converted, targetProfile);
  }
  if (!hasColorProfile(converted)) {
    applyDefaultColorProfile(converted);
  }
  return converted;
#else
  QImage tagged(image);
  applyIccProfile(tagged, targetProfile);
  if (!hasColorProfile(tagged)) {
    applyDefaultColorProfile(tagged);
  }
  return tagged;
#endif
}

}  // namespace color_profile
