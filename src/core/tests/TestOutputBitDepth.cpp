// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QColorSpace>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>

#include "ImageFileWriter.h"
#include "OutputFileFormatSettings.h"
#include "TiffReader.h"

namespace Tests {
namespace {
QImage makeImage(const QImage::Format format) {
  QImage image(40, 30, format);
  image.fill(QColor(200, 60, 30));
  image.setDotsPerMeterX(23622);
  image.setDotsPerMeterY(23622);
  image.setColorSpace(QColorSpace::AdobeRgb);
  return image;
}

QImage readTiff(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return TiffReader::readImage(file);
}

/** Restores the global output settings after a test changes them. */
struct OutputSettingsGuard {
  ~OutputSettingsGuard() { OutputFileFormatSettings::getInstance().resetToDefault(); }
};
}  // namespace

BOOST_AUTO_TEST_SUITE(OutputBitDepthTestSuite)

BOOST_AUTO_TEST_CASE(test_conversion_formats) {
  const QImage color8 = makeImage(QImage::Format_RGB32);
  const QImage color16 = makeImage(QImage::Format_RGBX64);
  const QImage gray8 = makeImage(QImage::Format_Grayscale8);
  const QImage gray16 = makeImage(QImage::Format_Grayscale16);
  const QImage mono = makeImage(QImage::Format_Mono);

  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(color8, OutputBitDepth::Bits16).format(),
                    QImage::Format_RGBX64);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(color16, OutputBitDepth::Bits8).format(), QImage::Format_RGB32);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(gray8, OutputBitDepth::Bits16).format(),
                    QImage::Format_Grayscale16);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(gray16, OutputBitDepth::Bits8).format(),
                    QImage::Format_Grayscale8);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(mono, OutputBitDepth::Bits16).format(), QImage::Format_Mono);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(color16, OutputBitDepth::Source).format(),
                    QImage::Format_RGBX64);
  BOOST_CHECK_EQUAL(ImageFileWriter::convertToBitDepth(color16, OutputBitDepth::Bits16).format(),
                    QImage::Format_RGBX64);

  const QImage converted = ImageFileWriter::convertToBitDepth(color8, OutputBitDepth::Bits16);
  BOOST_CHECK_EQUAL(converted.dotsPerMeterX(), color8.dotsPerMeterX());
  BOOST_CHECK(converted.colorSpace() == color8.colorSpace());
  BOOST_CHECK(converted.pixelColor(3, 3) == color8.pixelColor(3, 3));
}

BOOST_AUTO_TEST_CASE(test_tiff_written_with_requested_bit_depth) {
  OutputSettingsGuard guard;
  QTemporaryDir dir;
  BOOST_REQUIRE(dir.isValid());
  OutputFileFormatSettings& settings = OutputFileFormatSettings::getInstance();
  settings.resetToDefault();

  const QString to16 = dir.filePath("to16.tif");
  settings.setBitDepth(OutputBitDepth::Bits16);
  BOOST_REQUIRE(ImageFileWriter::writeImage(to16, makeImage(QImage::Format_RGB32), OutputFileFormat::Tiff));
  BOOST_CHECK_EQUAL(readTiff(to16).depth(), 64);

  const QImage to8Image = makeImage(QImage::Format_RGBX64);
  const QString to8 = dir.filePath("to8.tif");
  settings.setBitDepth(OutputBitDepth::Bits8);
  BOOST_REQUIRE(ImageFileWriter::writeImage(to8, to8Image, OutputFileFormat::Tiff));
  const QImage read8 = readTiff(to8);
  BOOST_CHECK_EQUAL(read8.depth(), 32);
  BOOST_CHECK_EQUAL(read8.dotsPerMeterX(), to8Image.dotsPerMeterX());

  const QString kept = dir.filePath("kept.tif");
  settings.setBitDepth(OutputBitDepth::Source);
  BOOST_REQUIRE(ImageFileWriter::writeImage(kept, to8Image, OutputFileFormat::Tiff));
  BOOST_CHECK_EQUAL(readTiff(kept).depth(), 64);
}

BOOST_AUTO_TEST_CASE(test_bit_depth_round_trips_as_string) {
  for (const OutputBitDepth bitDepth : {OutputBitDepth::Source, OutputBitDepth::Bits8, OutputBitDepth::Bits16}) {
    OutputBitDepth parsed = OutputBitDepth::Source;
    BOOST_REQUIRE(outputBitDepthFromString(outputBitDepthToString(bitDepth), &parsed));
    BOOST_CHECK(parsed == bitDepth);
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
