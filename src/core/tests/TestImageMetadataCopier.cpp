// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>

#include "ImageMetadataCopier.h"

namespace Tests {
namespace {
QImage makeImage() {
  QImage image(64, 48, QImage::Format_RGB32);
  image.fill(QColor(200, 120, 40));
  image.setDotsPerMeterX(11811);
  image.setDotsPerMeterY(11811);
  return image;
}

/** An EXIF APP1 segment with Make = "TestMake" and Orientation = 6 (rotated). */
QByteArray makeExifApp1() {
  QByteArray tiff;
  tiff.append("II*\0", 4);
  tiff.append("\x08\0\0\0", 4);  // IFD0 offset
  tiff.append("\x02\0", 2);      // 2 entries
  // Make, ASCII, count 9, value at offset 38
  tiff.append("\x0f\x01\x02\0\x09\0\0\0\x26\0\0\0", 12);
  // Orientation, SHORT, count 1, value 6
  tiff.append("\x12\x01\x03\0\x01\0\0\0\x06\0\0\0", 12);
  tiff.append("\0\0\0\0", 4);  // no next IFD
  tiff.append("TestMake\0", 9);

  QByteArray payload("Exif\0\0", 6);
  payload.append(tiff);

  QByteArray segment("\xff\xe1", 2);
  const int length = payload.size() + 2;
  segment.append(char((length >> 8) & 0xff));
  segment.append(char(length & 0xff));
  segment.append(payload);
  return segment;
}

bool writeImage(const QString& path, const char* format) {
  QImageWriter writer(path, format);
  return writer.write(makeImage());
}

bool writeJpegWithExif(const QString& path) {
  QByteArray jpeg;
  QBuffer buffer(&jpeg);
  buffer.open(QIODevice::WriteOnly);
  if (!makeImage().save(&buffer, "JPEG", 95)) {
    return false;
  }
  jpeg.insert(2, makeExifApp1());  // right after SOI

  QFile file(path);
  return file.open(QIODevice::WriteOnly) && (file.write(jpeg) == jpeg.size());
}

QByteArray readAll(const QString& path) {
  QFile file(path);
  file.open(QIODevice::ReadOnly);
  return file.readAll();
}
}  // namespace

BOOST_AUTO_TEST_SUITE(ImageMetadataCopierTestSuite)

BOOST_AUTO_TEST_CASE(test_copies_exif_and_keeps_pixels) {
  if (!image_metadata::isCopySupported()) {
    BOOST_TEST_MESSAGE("Built without Exiv2, skipping.");
    return;
  }

  QTemporaryDir dir;
  BOOST_REQUIRE(dir.isValid());
  const QString source = dir.filePath("source.jpg");
  BOOST_REQUIRE(writeJpegWithExif(source));

  for (const char* format : {"tiff", "png", "jpeg"}) {
    if (!QImageWriter::supportedImageFormats().contains(format)) {
      BOOST_TEST_MESSAGE("Qt cannot write " << format << ", skipping it.");
      continue;  // e.g. the TIFF plugin is a separate package on some distributions
    }
    const QString target = dir.filePath(QStringLiteral("target.") + format);
    BOOST_REQUIRE(writeImage(target, format));
    const QImage before(target);

    QString error;
    BOOST_CHECK_MESSAGE(image_metadata::copyMetadata(source, target, &error), error.toStdString());
    // Exiv2 0.27 stores EXIF in PNG as a compressed "Raw profile type exif" text chunk.
    const QByteArray written = readAll(target);
    BOOST_CHECK_MESSAGE(written.contains("TestMake") || written.contains("Raw profile type exif"), format);

    const QImage after(target);
    BOOST_CHECK(after.convertToFormat(QImage::Format_RGB32) == before.convertToFormat(QImage::Format_RGB32));
  }
}

BOOST_AUTO_TEST_CASE(test_source_without_metadata_leaves_target_untouched) {
  QTemporaryDir dir;
  BOOST_REQUIRE(dir.isValid());
  const QString source = dir.filePath("plain.png");
  const QString target = dir.filePath("target.png");
  BOOST_REQUIRE(writeImage(source, "png"));
  BOOST_REQUIRE(writeImage(target, "png"));
  const QByteArray before = readAll(target);

  if (image_metadata::isCopySupported()) {
    BOOST_CHECK(image_metadata::copyMetadata(source, target));
  }
  BOOST_CHECK(readAll(target) == before);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
