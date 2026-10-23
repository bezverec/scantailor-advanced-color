// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_SYSTEMMEMORY_H_
#define SCANTAILOR_CORE_SYSTEMMEMORY_H_

#include <QSize>
#include <QtGlobal>

#include "Dpi.h"

/**
 * Memory-aware limits for batch processing.
 *
 * Batch processing used to start as many pages as there are CPU cores. Large 16-bit
 * pages need gigabytes each, so that could exhaust memory (Windows reports out of
 * memory, other systems start swapping heavily). These helpers let the scheduler
 * start another page only when there is room for it.
 */
namespace system_memory {
/** Physical memory currently available for new allocations, in bytes, or 0 if unknown. */
quint64 availableBytes();

/**
 * Conservative estimate of the peak memory needed to process one page.
 *
 * The bit depth is not known before the image is loaded, so 16-bit RGBA (8 bytes per
 * pixel) is assumed. \p outputDpi is null unless the batch runs up to the Output stage.
 */
quint64 estimatePageBytes(const QSize& imageSize, const Dpi& imageDpi, const Dpi& outputDpi);

/**
 * Whether one more page needing \p neededBytes can start while pages reserving
 * \p reservedBytes in total are already running. The first page is always allowed so
 * that batch processing cannot stall; an unknown amount of available memory (0)
 * disables the limit.
 */
bool canStartPage(quint64 availableBytes, quint64 reservedBytes, quint64 neededBytes, int runningPages);
}  // namespace system_memory

#endif  // SCANTAILOR_CORE_SYSTEMMEMORY_H_
