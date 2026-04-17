// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_RAWMETADATALOADER_H_
#define SCANTAILOR_CORE_RAWMETADATALOADER_H_

#include "ImageMetadataLoader.h"

class RawMetadataLoader : public ImageMetadataLoader {
 protected:
  Status loadMetadata(QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out) override;
};

#endif  // SCANTAILOR_CORE_RAWMETADATALOADER_H_
