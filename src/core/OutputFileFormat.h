// Copyright (C) 2026  OpenAI
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_OUTPUTFILEFORMAT_H_
#define SCANTAILOR_CORE_OUTPUTFILEFORMAT_H_

#include <QByteArray>
#include <QString>
#include <vector>

enum class OutputFileFormat { Tiff, Png, Jpeg };

QString outputFileFormatToString(OutputFileFormat format);

bool outputFileFormatFromString(const QString& str, OutputFileFormat* format);

QString outputFileFormatExtension(OutputFileFormat format);

QByteArray outputFileFormatQtWriterFormat(OutputFileFormat format);

bool outputFileFormatIsLossy(OutputFileFormat format);

std::vector<OutputFileFormat> allOutputFileFormats();

#endif  // SCANTAILOR_CORE_OUTPUTFILEFORMAT_H_
