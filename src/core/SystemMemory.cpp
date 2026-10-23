// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SystemMemory.h"

#include <algorithm>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#elif defined(Q_OS_LINUX)
#include <QFile>
#include <QRegularExpression>
#endif

namespace system_memory {
namespace {
const quint64 kBytesPerPixel = 8;  // 16-bit RGBA, the widest format the pipeline uses.
const quint64 kSafetyMarginBytes = 512ull * 1024 * 1024;

double dpiScale(const int from, const int to) {
  if ((from <= 0) || (to <= 0)) {
    return 1.0;
  }
  return double(to) / double(from);
}
}  // namespace

quint64 availableBytes() {
#if defined(Q_OS_WIN)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (!GlobalMemoryStatusEx(&status)) {
    return 0;
  }
  // Allocations fail once the commit limit is reached, even with physical memory left.
  return std::min<quint64>(status.ullAvailPhys, status.ullAvailPageFile);
#elif defined(Q_OS_MACOS)
  static const mach_port_t host = mach_host_self();
  vm_size_t pageSize = 0;
  if (host_page_size(host, &pageSize) != KERN_SUCCESS) {
    return 0;
  }
  vm_statistics64_data_t stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&stats), &count) != KERN_SUCCESS) {
    return 0;
  }
  const quint64 pages = quint64(stats.free_count) + stats.inactive_count + stats.purgeable_count
                        + stats.speculative_count;
  return pages * pageSize;
#elif defined(Q_OS_LINUX)
  QFile file(QStringLiteral("/proc/meminfo"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return 0;
  }
  static const QRegularExpression memAvailable(QStringLiteral("^MemAvailable:\\s+(\\d+)\\s+kB"),
                                               QRegularExpression::MultilineOption);
  const QRegularExpressionMatch match = memAvailable.match(QString::fromLatin1(file.readAll()));
  return match.hasMatch() ? match.captured(1).toULongLong() * 1024 : 0;
#else
  return 0;
#endif
}

quint64 estimatePageBytes(const QSize& imageSize, const Dpi& imageDpi, const Dpi& outputDpi) {
  if (imageSize.isEmpty()) {
    return 0;
  }
  const quint64 sourcePixels = quint64(imageSize.width()) * quint64(imageSize.height());
  // The source image plus a working copy of it.
  quint64 bytes = sourcePixels * kBytesPerPixel * 2;
  if (!outputDpi.isNull()) {
    const double scale = dpiScale(imageDpi.horizontal(), outputDpi.horizontal())
                         * dpiScale(imageDpi.vertical(), outputDpi.vertical());
    const auto outputPixels = quint64(double(sourcePixels) * scale);
    // Output image, its mask / mixed-mode companions and the colour-managed copy for writing.
    bytes += outputPixels * kBytesPerPixel * 3;
  }
  return bytes;
}

bool canStartPage(const quint64 availableBytes,
                  const quint64 reservedBytes,
                  const quint64 neededBytes,
                  const int runningPages) {
  if ((runningPages <= 0) || (availableBytes == 0)) {
    return true;
  }
  // Pages already running have allocated part of their reservation (which is then no
  // longer available) and may still need the rest, so half of it is kept aside.
  return availableBytes >= neededBytes + reservedBytes / 2 + kSafetyMarginBytes;
}
}  // namespace system_memory
