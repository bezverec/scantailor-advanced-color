// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEMETADATACOPIER_H_
#define SCANTAILOR_CORE_IMAGEMETADATACOPIER_H_

#include <QString>

/**
 * Copies descriptive EXIF / XMP / IPTC metadata from a source image to an
 * already written output image.
 *
 * Tags describing the pixel data (dimensions, compression, orientation,
 * resolution, ICC profile, thumbnails, DNG raw tags) keep the values the output
 * writer produced, so the copied metadata never contradicts the output pixels.
 */
namespace image_metadata {
/** Whether this build can copy metadata (built with Exiv2). */
bool isCopySupported();

/**
 * Copies metadata from \p sourcePath into \p targetPath in place.
 * Returns false and fills \p errorMessage if the target could not be updated.
 * A source without any metadata is not an error.
 */
bool copyMetadata(const QString& sourcePath, const QString& targetPath, QString* errorMessage = nullptr);
}  // namespace image_metadata

#endif  // SCANTAILOR_CORE_IMAGEMETADATACOPIER_H_
