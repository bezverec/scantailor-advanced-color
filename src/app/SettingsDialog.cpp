// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SettingsDialog.h"

#include <core/ApplicationSettings.h>
#include <core/RawSettings.h>
#include <tiff.h>

#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include <cmath>

#include "Application.h"
#include "OpenGLSupport.h"
#include "config.h"

SettingsDialog::SettingsDialog(const bool rawSettingsEnabled, QWidget* parent)
    : QDialog(parent), m_rawSettingsEnabled(rawSettingsEnabled) {
  ui.setupUi(this);

  ApplicationSettings& settings = ApplicationSettings::getInstance();

  if (!OpenGLSupport::supported()) {
    ui.enableOpenglCb->setChecked(false);
    ui.enableOpenglCb->setEnabled(false);
    ui.openglDeviceLabel->setEnabled(false);
    ui.openglDeviceLabel->setText(tr("Your hardware / driver don't provide the necessary features"));
  } else {
    ui.enableOpenglCb->setChecked(settings.isOpenGlEnabled());
    const QString openglDevicePattern = ui.openglDeviceLabel->text();
    ui.openglDeviceLabel->setText(openglDevicePattern.arg(OpenGLSupport::deviceName()));
  }

  ui.colorSchemeBox->addItem(tr("Dark"), "dark");
  ui.colorSchemeBox->addItem(tr("Light"), "light");
  ui.colorSchemeBox->addItem(tr("Native"), "native");
  ui.colorSchemeBox->setCurrentIndex(ui.colorSchemeBox->findData(settings.getColorScheme()));
  connect(ui.colorSchemeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int) {
    QMessageBox::information(this, tr("Information"),
                             tr("ScanTailor need to be restarted to apply the color scheme changes."));
  });

  ui.tiffCompressionBWBox->addItem(tr("None"), COMPRESSION_NONE);
  ui.tiffCompressionBWBox->addItem(tr("LZW"), COMPRESSION_LZW);
  ui.tiffCompressionBWBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
  ui.tiffCompressionBWBox->addItem(tr("CCITT G4"), COMPRESSION_CCITTFAX4);
  ui.tiffCompressionBWBox->setCurrentIndex(ui.tiffCompressionBWBox->findData(settings.getTiffBwCompression()));

  ui.tiffCompressionColorBox->addItem(tr("None"), COMPRESSION_NONE);
  ui.tiffCompressionColorBox->addItem(tr("LZW"), COMPRESSION_LZW);
  ui.tiffCompressionColorBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
  ui.tiffCompressionColorBox->addItem(tr("JPEG"), COMPRESSION_JPEG);
  ui.tiffCompressionColorBox->setCurrentIndex(ui.tiffCompressionColorBox->findData(settings.getTiffColorCompression()));

  {
    auto* app = static_cast<Application*>(qApp);
    for (const QString& locale : app->getLanguagesList()) {
      QString languageName = QLocale::languageToString(QLocale(locale).language());
      ui.languageBox->addItem(languageName, locale);
    }
    ui.languageBox->setCurrentIndex(ui.languageBox->findData(app->getCurrentLocale()));
    ui.languageBox->setEnabled(ui.languageBox->count() > 1);
  }

  ui.blackOnWhiteDetectionCB->setChecked(settings.isBlackOnWhiteDetectionEnabled());
  ui.blackOnWhiteDetectionAtOutputCB->setEnabled(ui.blackOnWhiteDetectionCB->isChecked());
  ui.blackOnWhiteDetectionAtOutputCB->setChecked(settings.isBlackOnWhiteDetectionOutputEnabled());
  connect(ui.blackOnWhiteDetectionCB, SIGNAL(clicked(bool)), SLOT(blackOnWhiteDetectionToggled(bool)));

  ui.highlightDeviationCB->setChecked(settings.isHighlightDeviationEnabled());

  ui.deskewDeviationCoefSB->setValue(settings.getDeskewDeviationCoef());
  ui.deskewDeviationThresholdSB->setValue(settings.getDeskewDeviationThreshold());
  ui.selectContentDeviationCoefSB->setValue(settings.getSelectContentDeviationCoef());
  ui.selectContentDeviationThresholdSB->setValue(settings.getSelectContentDeviationThreshold());
  ui.marginsDeviationCoefSB->setValue(settings.getMarginsDeviationCoef());
  ui.marginsDeviationThresholdSB->setValue(settings.getMarginsDeviationThreshold());

  ui.autoSaveProjectCB->setChecked(settings.isAutoSaveProjectEnabled());

  ui.thumbnailQualitySB->setValue(settings.getThumbnailQuality().width());
  ui.thumbnailSizeSB->setValue(settings.getMaxLogicalThumbnailSize().toSize().width());

  ui.singleColumnThumbnailsCB->setChecked(settings.isSingleColumnThumbnailDisplayEnabled());
  ui.cancelingSelectionQuestionCB->setChecked(settings.isCancelingSelectionQuestionEnabled());

  ui.rawDemosaicBox->addItem(tr("Linear"), static_cast<int>(RawLoadParams::Demosaic::Linear));
  ui.rawDemosaicBox->addItem(tr("VNG"), static_cast<int>(RawLoadParams::Demosaic::VNG));
  ui.rawDemosaicBox->addItem(tr("PPG"), static_cast<int>(RawLoadParams::Demosaic::PPG));
  ui.rawDemosaicBox->addItem(tr("AHD"), static_cast<int>(RawLoadParams::Demosaic::AHD));
  ui.rawDemosaicBox->addItem(tr("DCB"), static_cast<int>(RawLoadParams::Demosaic::DCB));
  ui.rawDemosaicBox->addItem(tr("DHT"), static_cast<int>(RawLoadParams::Demosaic::DHT));

  ui.rawWhiteBalanceBox->addItem(tr("Camera"), static_cast<int>(RawLoadParams::WhiteBalance::Camera));
  ui.rawWhiteBalanceBox->addItem(tr("Auto"), static_cast<int>(RawLoadParams::WhiteBalance::Auto));
  ui.rawWhiteBalanceBox->addItem(tr("Daylight"), static_cast<int>(RawLoadParams::WhiteBalance::Daylight));
  ui.rawWhiteBalanceBox->addItem(tr("Manual"), static_cast<int>(RawLoadParams::WhiteBalance::Manual));

  const RawLoadParams& rawParams = RawSettings::getInstance().getParams();
  ui.rawDemosaicBox->setCurrentIndex(ui.rawDemosaicBox->findData(static_cast<int>(rawParams.demosaic)));
  ui.rawWhiteBalanceBox->setCurrentIndex(
      ui.rawWhiteBalanceBox->findData(static_cast<int>(rawParams.whiteBalance)));
  ui.rawExposureShiftSB->setValue(rawParams.exposureShift);
  ui.rawUseCameraMatrixCB->setChecked(rawParams.useCameraMatrix);
  ui.rawWbRedSB->setValue(rawParams.wbMult[0]);
  ui.rawWbGreenSB->setValue(rawParams.wbMult[1]);
  ui.rawWbBlueSB->setValue(rawParams.wbMult[2]);
  ui.rawWbGreen2SB->setValue(rawParams.wbMult[3]);

#ifdef HAVE_LIBRAW
  const bool rawUiEnabled = m_rawSettingsEnabled;
  ui.rawSettingsHintLabel->setText(rawUiEnabled ? tr("These settings are saved into the current project.")
                                                : tr("Open a project to edit RAW / DNG import settings."));
#else
  const bool rawUiEnabled = false;
  ui.rawSettingsHintLabel->setText(tr("LibRaw support is not available in this build."));
#endif
  ui.rawImportGroup->setEnabled(rawUiEnabled);
  connect(ui.rawWhiteBalanceBox, SIGNAL(currentIndexChanged(int)), SLOT(rawWhiteBalanceChanged(int)));
  updateRawSettingsUiState();

  connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::commitChanges() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();

  settings.setOpenGlEnabled(ui.enableOpenglCb->isChecked());
  settings.setAutoSaveProjectEnabled(ui.autoSaveProjectCB->isChecked());
  settings.setHighlightDeviationEnabled(ui.highlightDeviationCB->isChecked());
  settings.setColorScheme(ui.colorSchemeBox->currentData().toString());

  settings.setTiffBwCompression(ui.tiffCompressionBWBox->currentData().toInt());
  settings.setTiffColorCompression(ui.tiffCompressionColorBox->currentData().toInt());
  settings.setLanguage(ui.languageBox->currentData().toString());

  settings.setDeskewDeviationCoef(ui.deskewDeviationCoefSB->value());
  settings.setDeskewDeviationThreshold(ui.deskewDeviationThresholdSB->value());
  settings.setSelectContentDeviationCoef(ui.selectContentDeviationCoefSB->value());
  settings.setSelectContentDeviationThreshold(ui.selectContentDeviationThresholdSB->value());
  settings.setMarginsDeviationCoef(ui.marginsDeviationCoefSB->value());
  settings.setMarginsDeviationThreshold(ui.marginsDeviationThresholdSB->value());

  settings.setBlackOnWhiteDetectionEnabled(ui.blackOnWhiteDetectionCB->isChecked());
  settings.setBlackOnWhiteDetectionOutputEnabled(ui.blackOnWhiteDetectionAtOutputCB->isChecked());

  {
    const int quality = ui.thumbnailQualitySB->value();
    settings.setThumbnailQuality(QSize(quality, quality));
  }
  {
    const double width = ui.thumbnailSizeSB->value();
    const double height = std::round((width * (16.0 / 25.0)) * 100) / 100;
    settings.setMaxLogicalThumbnailSize(QSizeF(width, height));
  }

  settings.setSingleColumnThumbnailDisplayEnabled(ui.singleColumnThumbnailsCB->isChecked());
  settings.setCancelingSelectionQuestionEnabled(ui.cancelingSelectionQuestionCB->isChecked());

  if (ui.rawImportGroup->isEnabled()) {
    RawLoadParams params = RawSettings::getInstance().getParams();
    params.demosaic = static_cast<RawLoadParams::Demosaic>(ui.rawDemosaicBox->currentData().toInt());
    params.whiteBalance = static_cast<RawLoadParams::WhiteBalance>(ui.rawWhiteBalanceBox->currentData().toInt());
    params.exposureShift = static_cast<float>(ui.rawExposureShiftSB->value());
    params.useCameraMatrix = ui.rawUseCameraMatrixCB->isChecked();
    params.wbMult[0] = static_cast<float>(ui.rawWbRedSB->value());
    params.wbMult[1] = static_cast<float>(ui.rawWbGreenSB->value());
    params.wbMult[2] = static_cast<float>(ui.rawWbBlueSB->value());
    params.wbMult[3] = static_cast<float>(ui.rawWbGreen2SB->value());

    RawSettings& rawSettings = RawSettings::getInstance();
    if (params != rawSettings.getParams()) {
      rawSettings.setParams(params);
      emit rawSettingsChanged();
    }
  }

  emit settingsChanged();
}

void SettingsDialog::blackOnWhiteDetectionToggled(bool checked) {
  ui.blackOnWhiteDetectionAtOutputCB->setEnabled(checked);
}

void SettingsDialog::rawWhiteBalanceChanged(int idx) {
  Q_UNUSED(idx);
  updateRawSettingsUiState();
}

void SettingsDialog::updateRawSettingsUiState() {
  const bool manualWhiteBalance
      = ui.rawWhiteBalanceBox->currentData().toInt() == static_cast<int>(RawLoadParams::WhiteBalance::Manual);
  ui.rawManualWbWidget->setEnabled(ui.rawImportGroup->isEnabled() && manualWhiteBalance);
}
