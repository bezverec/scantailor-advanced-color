// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QColorSpace>
#include <QImage>
#include <boost/test/unit_test.hpp>

#include "ColorProfileUtils.h"
#include "OutputFileFormatSettings.h"

namespace Tests {
namespace {
QImage makeImage(const QImage::Format format) {
  QImage image(40, 30, format);
  image.fill(QColor(200, 60, 30));
  image.setDotsPerMeterX(23622);  // 600 DPI
  image.setDotsPerMeterY(11811);  // 300 DPI
  image.setText("Description", "test");
  return image;
}
}  // namespace

BOOST_AUTO_TEST_SUITE(ColorProfileUtilsTestSuite)

BOOST_AUTO_TEST_CASE(test_profile_conversion_keeps_format_and_resolution) {
  for (const QImage::Format format : {QImage::Format_RGBX64, QImage::Format_RGB32, QImage::Format_RGB888}) {
    const QImage source = makeImage(format);
    const QImage converted = color_profile::prepareForOutput(
        source, OutputColorProfileMode::AdobeRgb, OutputRenderingIntent::RelativeColorimetric, QString());

    BOOST_REQUIRE(!converted.isNull());
    BOOST_CHECK_EQUAL(converted.format(), format);
    BOOST_CHECK(converted.size() == source.size());
    BOOST_CHECK_EQUAL(converted.dotsPerMeterX(), source.dotsPerMeterX());
    BOOST_CHECK_EQUAL(converted.dotsPerMeterY(), source.dotsPerMeterY());
    BOOST_CHECK(converted.text("Description") == "test");
    BOOST_CHECK(converted.colorSpace().isValid());
#ifdef SCANTAILOR_HAVE_LCMS2
    // sRGB red is less saturated in Adobe RGB, so the pixel values must change.
    BOOST_CHECK(converted.pixelColor(5, 5) != source.pixelColor(5, 5));
#endif
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
