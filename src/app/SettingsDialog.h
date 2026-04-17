// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_SETTINGSDIALOG_H_
#define SCANTAILOR_APP_SETTINGSDIALOG_H_

#include <QDialog>

#include "ui_SettingsDialog.h"

class SettingsDialog : public QDialog {
  Q_OBJECT
 public:
  explicit SettingsDialog(bool rawSettingsEnabled, QWidget* parent = nullptr);

  ~SettingsDialog() override;

 signals:
  void settingsChanged();

  void rawSettingsChanged();

 private slots:
  void commitChanges();

  void blackOnWhiteDetectionToggled(bool checked);

  void rawWhiteBalanceChanged(int idx);

 private:
  void updateRawSettingsUiState();

  Ui::SettingsDialog ui;
  bool m_rawSettingsEnabled;
};


#endif
