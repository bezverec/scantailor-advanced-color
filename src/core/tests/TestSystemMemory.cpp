// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <boost/test/unit_test.hpp>

#include "SystemMemory.h"

namespace Tests {
namespace {
const quint64 MiB = 1024ull * 1024;
const quint64 GiB = 1024 * MiB;
}  // namespace

BOOST_AUTO_TEST_SUITE(SystemMemoryTestSuite)

BOOST_AUTO_TEST_CASE(test_estimate_grows_with_output_dpi) {
  const QSize size(4000, 5000);  // 20 Mpx
  const Dpi dpi(400, 400);

  const quint64 withoutOutput = system_memory::estimatePageBytes(size, dpi, Dpi());
  const quint64 sameDpi = system_memory::estimatePageBytes(size, dpi, Dpi(400, 400));
  const quint64 upscaled = system_memory::estimatePageBytes(size, dpi, Dpi(800, 800));

  BOOST_CHECK_EQUAL(withoutOutput, 20000000ull * 8 * 2);
  BOOST_CHECK_EQUAL(sameDpi, withoutOutput + 20000000ull * 8 * 3);
  // Doubling the DPI quadruples the output pixels.
  BOOST_CHECK_EQUAL(upscaled, withoutOutput + 4 * 20000000ull * 8 * 3);
  BOOST_CHECK_EQUAL(system_memory::estimatePageBytes(QSize(), dpi, dpi), 0u);
}

BOOST_AUTO_TEST_CASE(test_first_page_always_starts) {
  BOOST_CHECK(system_memory::canStartPage(100 * MiB, 0, 10 * GiB, 0));
}

BOOST_AUTO_TEST_CASE(test_unknown_available_memory_does_not_limit) {
  BOOST_CHECK(system_memory::canStartPage(0, 50 * GiB, 10 * GiB, 8));
}

BOOST_AUTO_TEST_CASE(test_further_pages_need_room) {
  // 2 GiB needed + half of 4 GiB reserved + 512 MiB margin = 4.5 GiB.
  BOOST_CHECK(system_memory::canStartPage(5 * GiB, 4 * GiB, 2 * GiB, 2));
  BOOST_CHECK(!system_memory::canStartPage(4 * GiB, 4 * GiB, 2 * GiB, 2));
}

BOOST_AUTO_TEST_CASE(test_available_memory_is_reported) {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
  BOOST_CHECK(system_memory::availableBytes() > 0);
#endif
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
