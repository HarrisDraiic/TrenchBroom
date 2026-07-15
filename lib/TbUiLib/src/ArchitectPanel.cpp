/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "ui/ArchitectPanel.h"

#include <QByteArray>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

#include "architect/Error.h"
#include "architect/Protocol.h"
#include "mdl/ArchitectRoomBuilder.h"
#include "mdl/Map.h"
#include "ui/ArchitectBranding.h"
#include "ui/MapDocument.h"

#include "kd/overload.h"

#include <string>
#include <utility>
#include <vector>

namespace tb::ui
{
namespace
{

constexpr double DefaultUnitsPerMetre = 32.0;
constexpr int OperationTimeoutMilliseconds = 10'000;

QString runtimeExecutablePath()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  auto filename = QString::fromUtf8(ArchitectBranding::RuntimeLinuxExecutableName);
#else
  auto filename = QString::fromUtf8(ArchitectBranding::RuntimeExecutableName);
#endif
#if defined(Q_OS_WIN)
  filename += ".exe";
#endif
  return QDir{QCoreApplication::applicationDirPath()}.filePath(filename);
}

QString errorCodeString(const architect::ErrorCode code)
{
  const auto name = architect::errorCodeName(code);
  return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

} // namespace

ArchitectPanel::ArchitectPanel(MapDocument& document, QWidget* parent)
  : QWidget{parent}
  , m_document{document}
  , m_process{new QProcess{this}}
  , m_timeout{new QTimer{this}}
{
  createGui();
  connectGui();

  m_timeout->setSingleShot(true);
  m_timeout->setInterval(OperationTimeoutMilliseconds);

  appendArchitectMessage(
    "Mock provider ready. Plan a dimensioned room; no map changes occur until "
    "write access is enabled and Apply Blueprint is pressed.");

  QTimer::singleShot(0, this, [this]() { startRuntime(); });
}

ArchitectPanel::~ArchitectPanel()
{
  if (m_process->state() != QProcess::NotRunning)
  {
    m_process->kill();
    m_process->waitForFinished(1000);
  }
}

void ArchitectPanel::createGui()
{
  m_status = new QLabel{"Runtime: starting"};
  m_status->setObjectName("ArchitectPanel_Status");

  m_transcript = new QTextBrowser{};
  m_transcript->setObjectName("ArchitectPanel_Transcript");
  m_transcript->setOpenExternalLinks(false);

  m_prompt = new QPlainTextEdit{};
  m_prompt->setObjectName("ArchitectPanel_Prompt");
  m_prompt->setPlaceholderText(
    "Create a stone room 12 metres wide, 10 metres deep and 5 metres tall, "
    "with one doorway centered on the southern wall.");
  m_prompt->setMaximumHeight(100);

  m_writeEnabled = new QCheckBox{"Enable map write access for Apply Blueprint"};
  m_writeEnabled->setObjectName("ArchitectPanel_WriteEnabled");
  m_writeEnabled->setChecked(false);

  m_planButton = new QPushButton{"Plan Room"};
  m_planButton->setObjectName("ArchitectPanel_PlanRoom");
  m_applyButton = new QPushButton{"Apply Blueprint"};
  m_applyButton->setObjectName("ArchitectPanel_ApplyBlueprint");
  m_stopButton = new QPushButton{"Stop"};
  m_stopButton->setObjectName("ArchitectPanel_Stop");

  auto* buttonLayout = new QHBoxLayout{};
  buttonLayout->addWidget(m_planButton);
  buttonLayout->addWidget(m_applyButton);
  buttonLayout->addWidget(m_stopButton);

  auto* layout = new QVBoxLayout{};
  layout->addWidget(new QLabel{"Provider: deterministic mock (no credentials)"});
  layout->addWidget(new QLabel{"Scale: 32 map units per metre"});
  layout->addWidget(m_status);
  layout->addWidget(m_transcript, 1);
  layout->addWidget(m_prompt);
  layout->addWidget(m_writeEnabled);
  layout->addLayout(buttonLayout);
  setLayout(layout);

  updateButtonState();
}

void ArchitectPanel::connectGui()
{
  connect(m_planButton, &QPushButton::clicked, this, [this]() { planRoom(); });
  connect(m_applyButton, &QPushButton::clicked, this, [this]() { applyRoom(); });
  connect(m_stopButton, &QPushButton::clicked, this, [this]() {
    stopRuntime("operation_cancelled");
  });
  connect(m_writeEnabled, &QCheckBox::toggled, this, [this]() { updateButtonState(); });

  connect(m_process, &QProcess::started, this, [this]() {
    m_status->setText("Runtime: connected locally by private child-process pipes");
    updateButtonState();
  });
  connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
    processStandardOutput();
  });
  connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
    // The runtime emits no expected stderr. Do not surface raw child output because it
    // may contain implementation details in a future provider implementation.
    m_process->readAllStandardError();
  });
  connect(
    m_process,
    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
    this,
    [this](const int, const QProcess::ExitStatus) {
      m_timeout->stop();
      m_pendingRequestId.clear();
      m_status->setText("Runtime: stopped");
      updateButtonState();
      if (m_restartAfterStop)
      {
        m_restartAfterStop = false;
        QTimer::singleShot(0, this, [this]() { startRuntime(); });
      }
    });
  connect(m_process, &QProcess::errorOccurred, this, [this](const auto) {
    m_status->setText("Runtime: unavailable");
    appendError(
      "runtime_unavailable",
      "The bundled Architect runtime could not be started. Reinstall from a complete "
      "Architect package.");
    updateButtonState();
  });
  connect(m_timeout, &QTimer::timeout, this, [this]() {
    appendError("operation_timeout", "The runtime did not respond within 10 seconds.");
    stopRuntime("operation_timeout");
  });
}

void ArchitectPanel::startRuntime()
{
  if (m_process->state() != QProcess::NotRunning)
  {
    return;
  }

  m_responseBuffer.clear();
  m_status->setText("Runtime: starting");
  m_process->setProgram(runtimeExecutablePath());
  m_process->setArguments({});
  m_process->start(QIODevice::ReadWrite);
  updateButtonState();
}

void ArchitectPanel::stopRuntime(QString reason)
{
  m_timeout->stop();
  if (!m_pendingRequestId.isEmpty() && reason == "operation_cancelled")
  {
    appendError(std::move(reason), "The current planning request was cancelled.");
  }
  m_pendingRequestId.clear();
  m_pendingBlueprint.reset();
  if (m_process->state() != QProcess::NotRunning)
  {
    m_restartAfterStop = true;
    m_process->kill();
  }
  updateButtonState();
  QTimer::singleShot(0, this, [this]() { startRuntime(); });
}

void ArchitectPanel::updateButtonState()
{
  const auto connected = m_process->state() == QProcess::Running;
  const auto busy = !m_pendingRequestId.isEmpty();
  m_planButton->setEnabled(connected && !busy);
  m_applyButton->setEnabled(
    !busy && m_writeEnabled->isChecked() && m_pendingBlueprint.has_value());
  m_stopButton->setEnabled(connected && busy);
}

void ArchitectPanel::planRoom()
{
  const auto prompt = m_prompt->toPlainText().trimmed();
  if (prompt.isEmpty())
  {
    appendError("invalid_argument", "Enter a room request first.");
    return;
  }
  if (m_process->state() != QProcess::Running)
  {
    appendError("runtime_unavailable", "The bundled runtime is not connected.");
    startRuntime();
    return;
  }

  m_pendingBlueprint.reset();
  m_pendingRequestId = QString{"room-%1"}.arg(m_nextRequestId++);
  appendUserMessage(prompt);

  const auto request = QJsonObject{
    {"protocol", "architect/1"},
    {"id", m_pendingRequestId},
    {"method", "plan.room"},
    {"params",
     QJsonObject{
       {"prompt", prompt},
       {"units_per_metre", DefaultUnitsPerMetre},
       {"material", QString::fromStdString(m_document.map().currentMaterialName())},
     }},
  };
  auto bytes = QJsonDocument{request}.toJson(QJsonDocument::Compact);
  bytes.append('\n');
  if (
    bytes.size() > static_cast<qsizetype>(architect::MaximumRequestBytes)
    || m_process->write(bytes) != bytes.size())
  {
    m_pendingRequestId.clear();
    appendError("invalid_request", "The request could not be sent safely.");
    updateButtonState();
    return;
  }

  m_timeout->start();
  updateButtonState();
}

void ArchitectPanel::applyRoom()
{
  if (!m_writeEnabled->isChecked())
  {
    appendError("write_disabled", "Enable map write access before applying a blueprint.");
    return;
  }
  if (!m_pendingBlueprint)
  {
    appendError("invalid_blueprint", "Plan a valid room before applying it.");
    return;
  }

  auto result = mdl::createArchitectRoom(m_document.map(), *m_pendingBlueprint);
  if (result.is_error())
  {
    const auto error = result.visit(kdl::overload(
      [](const std::vector<mdl::BrushNode*>&) {
        return architect::Error{
          architect::ErrorCode::InternalError,
          "Unexpected room result.",
        };
      },
      [](const architect::Error& value) { return value; }));
    appendError(errorCodeString(error.code), QString::fromStdString(error.message));
    return;
  }

  appendArchitectMessage(QString{"Applied %1 brushes as one undoable transaction."}.arg(
    static_cast<qsizetype>(result.value().size())));
  m_pendingBlueprint.reset();
  m_writeEnabled->setChecked(false);
  updateButtonState();
}

void ArchitectPanel::processStandardOutput()
{
  m_responseBuffer.append(m_process->readAllStandardOutput());
  if (m_responseBuffer.size() > static_cast<qsizetype>(architect::MaximumRequestBytes))
  {
    appendError("invalid_response", "The runtime response exceeded the safety limit.");
    stopRuntime("invalid_response");
    return;
  }

  auto newline = m_responseBuffer.indexOf('\n');
  while (newline >= 0)
  {
    const auto line = m_responseBuffer.left(newline);
    m_responseBuffer.remove(0, newline + 1);
    if (!line.trimmed().isEmpty())
    {
      handleResponse(line);
    }
    newline = m_responseBuffer.indexOf('\n');
  }
}

void ArchitectPanel::handleResponse(const QByteArray& line)
{
  auto parseError = QJsonParseError{};
  const auto document = QJsonDocument::fromJson(line, &parseError);
  if (
    parseError.error != QJsonParseError::NoError || !document.isObject()
    || document.object().value("id").toString() != m_pendingRequestId)
  {
    appendError("invalid_response", "The runtime returned an invalid response.");
    stopRuntime("invalid_response");
    return;
  }

  m_timeout->stop();
  if (document.object().value("protocol").toString() != "architect/1")
  {
    appendError("unsupported_protocol", "The runtime response protocol is unsupported.");
    stopRuntime("unsupported_protocol");
    return;
  }
  m_pendingRequestId.clear();
  const auto response = document.object();
  const auto error = response.value("error").toObject();
  if (!error.isEmpty())
  {
    appendError(error.value("code").toString(), error.value("message").toString());
    updateButtonState();
    return;
  }

  const auto blueprintJson =
    response.value("result").toObject().value("blueprint").toObject();
  auto parsed = architect::roomBlueprintFromJson(blueprintJson);
  if (const auto* blueprint = std::get_if<architect::RoomBlueprint>(&parsed))
  {
    m_pendingBlueprint = *blueprint;
    appendArchitectMessage(
      QString{"Blueprint ready: %1 m wide x %2 m deep x %3 m tall; 8 deterministic "
              "brushes; one centered south doorway. Review it, enable write access, then "
              "Apply Blueprint."}
        .arg(blueprint->interiorWidth / blueprint->unitsPerMetre, 0, 'f', 1)
        .arg(blueprint->interiorDepth / blueprint->unitsPerMetre, 0, 'f', 1)
        .arg(blueprint->interiorHeight / blueprint->unitsPerMetre, 0, 'f', 1));
  }
  else
  {
    const auto& value = std::get<architect::Error>(parsed);
    appendError(errorCodeString(value.code), QString::fromStdString(value.message));
  }
  updateButtonState();
}

void ArchitectPanel::appendUserMessage(const QString& message)
{
  m_transcript->append("<b>You</b><br>" + message.toHtmlEscaped());
}

void ArchitectPanel::appendArchitectMessage(const QString& message)
{
  m_transcript->append("<b>Architect</b><br>" + message.toHtmlEscaped());
}

void ArchitectPanel::appendError(const QString& code, const QString& message)
{
  m_transcript->append(
    "<b>Architect error [" + code.toHtmlEscaped() + "]</b><br>"
    + message.toHtmlEscaped());
}

} // namespace tb::ui
