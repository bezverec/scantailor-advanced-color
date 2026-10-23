// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorProfileUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSysInfo>
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

/** lcms2 pixel type that reads and writes \p format as is, or 0 if the image has to be converted first. */
cmsUInt32Number directLcmsPixelType(const QImage::Format format) {
  switch (format) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
      return TYPE_RGBA_16;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
      return TYPE_RGBA_8;
#endif
    case QImage::Format_RGB888:
      return TYPE_RGB_8;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      // 0xAARRGGBB words: B, G, R, A in memory on little-endian machines.
      return (QSysInfo::ByteOrder == QSysInfo::LittleEndian) ? TYPE_BGRA_8 : TYPE_ARGB_8;
    default:
      return 0;
  }
}

/**
 * Converts \p image from \p srcProfile to \p dstProfile.
 *
 * Output pages can be several gigabytes, so formats lcms2 understands are transformed
 * straight into a single new image of the same format: one extra copy instead of
 * converting to RGBA64 and back. Resolution and text keys are carried over.
 */
QImage convertColorImageWithLcms(const QImage& image, const QByteArray& srcProfile, const QByteArray& dstProfile,
                                 const OutputRenderingIntent renderingIntent) {
  if (image.isNull() || srcProfile.isEmpty() || dstProfile.isEmpty() || srcProfile == dstProfile) {
    return image;
  }

  QImage input(image);
  cmsUInt32Number pixelType = directLcmsPixelType(input.format());
  const bool convertedInput = (pixelType == 0);
  if (convertedInput) {
    // Indexed, premultiplied and other formats lcms2 cannot read directly.
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    if (image.depth() > 8) {
      input = image.convertToFormat(QImage::Format_RGBA64);
    } else
#endif
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
      input = image.convertToFormat(QImage::Format_RGBA8888);
#else
      input = image.convertToFormat(QImage::Format_ARGB32);
#endif
    }
    pixelType = directLcmsPixelType(input.format());
  }

  QImage outImage(input.size(), input.format());
  if (input.isNull() || outImage.isNull() || (pixelType == 0)) {
    return image;
  }
  outImage.setDotsPerMeterX(image.dotsPerMeterX());
  outImage.setDotsPerMeterY(image.dotsPerMeterY());
  for (const QString& key : image.textKeys()) {
    outImage.setText(key, image.text(key));
  }

  cmsHPROFILE src = cmsOpenProfileFromMem(srcProfile.constData(), cmsUInt32Number(srcProfile.size()));
  cmsHPROFILE dst = cmsOpenProfileFromMem(dstProfile.constData(), cmsUInt32Number(dstProfile.size()));
  cmsHTRANSFORM transform = nullptr;
  if (src && dst) {
    transform = cmsCreateTransform(src, pixelType, dst, pixelType, lcmsIntent(renderingIntent), cmsFLAGS_COPY_ALPHA);
  }
  if (src) {
    cmsCloseProfile(src);
  }
  if (dst) {
    cmsCloseProfile(dst);
  }
  if (!transform) {
    return image;
  }

  const auto width = cmsUInt32Number(input.width());
  for (int y = 0; y < input.height(); ++y) {
    cmsDoTransform(transform, input.constScanLine(y), outImage.scanLine(y), width);
  }
  cmsDeleteTransform(transform);
  input = QImage();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  const QColorSpace dstColorSpace(QColorSpace::fromIccProfile(dstProfile));
  if (dstColorSpace.isValid()) {
    outImage.setColorSpace(dstColorSpace);
  }
#endif

  if (!convertedInput) {
    return outImage;
  }

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  if (image.depth() > 8) {
    if (image.hasAlphaChannel()) {
      return outImage;
    }
    return outImage.convertToFormat(QImage::Format_RGBX64);
  }
#endif

  if (image.hasAlphaChannel()) {
    return outImage;
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
