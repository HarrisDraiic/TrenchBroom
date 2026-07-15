/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

#include "architect/RoomBlueprint.h"

#include <optional>

class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTextBrowser;
class QTimer;

namespace tb::ui
{
class MapDocument;

class ArchitectPanel : public QWidget
{
private:
  MapDocument& m_document;
  QTextBrowser* m_transcript = nullptr;
  QLabel* m_status = nullptr;
  QPlainTextEdit* m_prompt = nullptr;
  QCheckBox* m_writeEnabled = nullptr;
  QPushButton* m_planButton = nullptr;
  QPushButton* m_applyButton = nullptr;
  QPushButton* m_stopButton = nullptr;
  QProcess* m_process = nullptr;
  QTimer* m_timeout = nullptr;

  QByteArray m_responseBuffer;
  QString m_pendingRequestId;
  std::optional<architect::RoomBlueprint> m_pendingBlueprint;
  int m_nextRequestId = 1;
  bool m_restartAfterStop = false;

public:
  explicit ArchitectPanel(MapDocument& document, QWidget* parent = nullptr);
  ~ArchitectPanel() override;

private:
  void createGui();
  void connectGui();
  void startRuntime();
  void stopRuntime(QString reason);
  void updateButtonState();

  void planRoom();
  void applyRoom();
  void processStandardOutput();
  void handleResponse(const QByteArray& line);

  void appendUserMessage(const QString& message);
  void appendArchitectMessage(const QString& message);
  void appendError(const QString& code, const QString& message);
};

} // namespace tb::ui
