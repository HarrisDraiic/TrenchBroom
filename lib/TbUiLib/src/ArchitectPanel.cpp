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
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
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
#include "ui/QPathUtils.h"
#include "ui/SystemPaths.h"

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

  m_profileChoice = new QComboBox{};
  m_profileChoice->setObjectName("ArchitectPanel_ProfileChoice");
  m_profileChoice->addItem("No profile", QString{});
  m_createProfileButton = new QPushButton{"Create Draft Profile"};
  m_createProfileButton->setObjectName("ArchitectPanel_CreateProfile");
  m_refreshProfilesButton = new QPushButton{"Refresh"};
  m_refreshProfilesButton->setObjectName("ArchitectPanel_RefreshProfiles");

  auto* profileLayout = new QHBoxLayout{};
  profileLayout->addWidget(new QLabel{"Profile:"});
  profileLayout->addWidget(m_profileChoice, 1);
  profileLayout->addWidget(m_createProfileButton);
  profileLayout->addWidget(m_refreshProfilesButton);

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
  layout->addLayout(profileLayout);
  layout->addWidget(
    new QLabel{"Active drafts guide explicit mock room rules; selection alone never "
               "edits the map."});
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
  connect(m_createProfileButton, &QPushButton::clicked, this, [this]() {
    createDraftProfile();
  });
  connect(m_refreshProfilesButton, &QPushButton::clicked, this, [this]() {
    requestProfiles();
  });
  connect(m_profileChoice, &QComboBox::currentIndexChanged, this, [this]() {
    selectProfile();
  });

  connect(m_process, &QProcess::started, this, [this]() {
    m_status->setText("Runtime: connected locally by private child-process pipes");
    updateButtonState();
    QTimer::singleShot(0, this, [this]() { requestProfiles(); });
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
      m_pendingMethod.clear();
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
  m_process->setArguments(
    {"--profile-root", pathAsQString(SystemPaths::architectProfilesDirectory())});
  m_process->start(QIODevice::ReadWrite);
  updateButtonState();
}

void ArchitectPanel::stopRuntime(QString reason)
{
  m_timeout->stop();
  if (!m_pendingRequestId.isEmpty() && reason == "operation_cancelled")
  {
    appendError(std::move(reason), "The current Architect operation was cancelled.");
  }
  m_pendingRequestId.clear();
  m_pendingMethod.clear();
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
  m_profileChoice->setEnabled(connected && !busy);
  m_createProfileButton->setEnabled(connected && !busy);
  m_refreshProfilesButton->setEnabled(connected && !busy);
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
  appendUserMessage(prompt);
  sendRequest(
    "plan.room",
    QJsonObject{
      {"prompt", prompt},
      {"units_per_metre", DefaultUnitsPerMetre},
      {"material", QString::fromStdString(m_document.map().currentMaterialName())},
    });
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

void ArchitectPanel::requestProfiles()
{
  if (m_process->state() != QProcess::Running || !m_pendingRequestId.isEmpty())
  {
    return;
  }
  sendRequest("profiles.list", QJsonObject{});
}

void ArchitectPanel::createDraftProfile()
{
  const auto designLanguage = m_prompt->toPlainText().trimmed();
  if (designLanguage.isEmpty())
  {
    appendError(
      "invalid_argument",
      "Describe the design language in the prompt box before creating a profile.");
    return;
  }

  auto accepted = false;
  const auto displayName =
    QInputDialog::getText(
      this, "Create Draft Profile", "Profile name:", QLineEdit::Normal, {}, &accepted)
      .trimmed();
  if (!accepted)
  {
    return;
  }
  if (displayName.isEmpty())
  {
    appendError("invalid_argument", "Enter a profile name.");
    return;
  }

  sendRequest(
    "profiles.create_draft",
    QJsonObject{
      {"display_name", displayName},
      {"design_language", designLanguage},
      {"aliases", QJsonArray{}},
    });
}

void ArchitectPanel::selectProfile()
{
  if (
    m_updatingProfiles || m_process->state() != QProcess::Running
    || !m_pendingRequestId.isEmpty())
  {
    return;
  }

  const auto reference = m_profileChoice->currentData().toString();
  if (reference.isEmpty())
  {
    sendRequest("profiles.clear_active", QJsonObject{});
  }
  else
  {
    sendRequest("profiles.set_active", QJsonObject{{"reference", reference}});
  }
}

void ArchitectPanel::sendRequest(QString method, QJsonObject params)
{
  if (m_process->state() != QProcess::Running)
  {
    appendError("runtime_unavailable", "The bundled runtime is not connected.");
    startRuntime();
    return;
  }
  if (!m_pendingRequestId.isEmpty())
  {
    appendError("operation_busy", "Wait for the current Architect operation to finish.");
    return;
  }

  m_pendingRequestId = QString{"request-%1"}.arg(m_nextRequestId++);
  m_pendingMethod = method;
  const auto request = QJsonObject{
    {"protocol", "architect/1"},
    {"id", m_pendingRequestId},
    {"method", std::move(method)},
    {"params", std::move(params)},
  };
  auto bytes = QJsonDocument{request}.toJson(QJsonDocument::Compact);
  bytes.append('\n');
  if (
    bytes.size() > static_cast<qsizetype>(architect::MaximumRequestBytes)
    || m_process->write(bytes) != bytes.size())
  {
    m_pendingRequestId.clear();
    m_pendingMethod.clear();
    appendError("invalid_request", "The request could not be sent safely.");
    updateButtonState();
    return;
  }

  m_timeout->start();
  updateButtonState();
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
  const auto method = std::exchange(m_pendingMethod, QString{});
  const auto response = document.object();
  const auto error = response.value("error").toObject();
  if (!error.isEmpty())
  {
    appendError(error.value("code").toString(), error.value("message").toString());
    updateButtonState();
    return;
  }

  const auto result = response.value("result").toObject();
  if (method == "profiles.list")
  {
    handleProfilesResponse(result);
    return;
  }
  if (method == "profiles.create_draft")
  {
    const auto profile = result.value("profile").toObject();
    const auto version = profile.value("version").toInt();
    if (
      profile.value("id").toString().isEmpty()
      || profile.value("display_name").toString().isEmpty()
      || profile.value("slug").toString().isEmpty()
      || profile.value("status").toString() != "draft" || version < 1)
    {
      appendError("invalid_response", "The runtime returned an invalid profile.");
      stopRuntime("invalid_response");
      return;
    }
    appendArchitectMessage(QString{"Saved draft profile %1 (version %2)."}
                             .arg(profile.value("display_name").toString())
                             .arg(version));
    requestProfiles();
    return;
  }
  if (method == "profiles.set_active")
  {
    const auto profile = result.value("profile").toObject();
    const auto displayName = profile.value("display_name").toString();
    if (displayName.isEmpty())
    {
      appendError("invalid_response", "The runtime returned an invalid profile.");
      stopRuntime("invalid_response");
      return;
    }
    appendArchitectMessage(
      QString{"Active profile: %1. Subsequent previews use supported read-only profile "
              "rules; selecting it does not change the map."}
        .arg(displayName));
    requestProfiles();
    return;
  }
  if (method == "profiles.clear_active")
  {
    if (!result.value("cleared").toBool())
    {
      appendError("invalid_response", "The runtime did not confirm profile selection.");
      stopRuntime("invalid_response");
      return;
    }
    appendArchitectMessage(
      "No active profile. Subsequent room previews use deterministic defaults.");
    requestProfiles();
    return;
  }
  if (method != "plan.room")
  {
    appendError("invalid_response", "The runtime response method was unexpected.");
    stopRuntime("invalid_response");
    return;
  }

  const auto blueprintJson = result.value("blueprint").toObject();
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
    appendArchitectMessage(QString{"Planning assumption: %1"}.arg(
      QString::fromStdString(blueprint->scaleAssumption)));
    if (blueprint->profile)
    {
      appendArchitectMessage(
        QString{"Profile provenance: %1 version %2 (%3); planned wall thickness %4 m."}
          .arg(QString::fromStdString(blueprint->profile->slug))
          .arg(blueprint->profile->version)
          .arg(QString::fromStdString(blueprint->profile->id))
          .arg(blueprint->wallThickness / blueprint->unitsPerMetre, 0, 'f', 2));
    }
    else
    {
      appendArchitectMessage(
        QString{"Profile provenance: none; planned wall thickness %1 m."}.arg(
          blueprint->wallThickness / blueprint->unitsPerMetre, 0, 'f', 2));
    }
  }
  else
  {
    const auto& value = std::get<architect::Error>(parsed);
    appendError(errorCodeString(value.code), QString::fromStdString(value.message));
  }
  updateButtonState();
}

void ArchitectPanel::handleProfilesResponse(const QJsonObject& result)
{
  const auto profilesValue = result.value("profiles");
  const auto activeProfileValue = result.value("active_profile_id");
  if (!profilesValue.isArray() || !activeProfileValue.isString())
  {
    appendError("invalid_response", "The runtime returned an invalid profile list.");
    stopRuntime("invalid_response");
    return;
  }

  struct ProfileChoice
  {
    QString id;
    QString displayName;
    QString slug;
  };
  auto profiles = std::vector<ProfileChoice>{};
  for (const auto& value : profilesValue.toArray())
  {
    const auto profile = value.toObject();
    const auto versionValue = profile.value("version");
    const auto version = versionValue.toInt();
    const auto choice = ProfileChoice{
      .id = profile.value("id").toString(),
      .displayName = profile.value("display_name").toString(),
      .slug = profile.value("slug").toString(),
    };
    if (
      !value.isObject() || choice.id.isEmpty() || choice.displayName.isEmpty()
      || choice.slug.isEmpty() || profile.value("status").toString() != "draft"
      || !versionValue.isDouble() || version < 1
      || versionValue.toDouble() != static_cast<double>(version))
    {
      appendError("invalid_response", "The runtime returned an invalid profile list.");
      stopRuntime("invalid_response");
      return;
    }
    profiles.push_back(choice);
  }

  const auto activeProfileId = activeProfileValue.toString();
  auto activeIndex = 0;
  m_updatingProfiles = true;
  m_profileChoice->clear();
  m_profileChoice->addItem("No profile", QString{});
  for (const auto& profile : profiles)
  {
    m_profileChoice->addItem(
      QString{"%1 (Draft)"}.arg(profile.displayName), profile.slug);
    const auto index = m_profileChoice->count() - 1;
    m_profileChoice->setItemData(index, profile.id, Qt::UserRole + 1);
    if (profile.id == activeProfileId)
    {
      activeIndex = index;
    }
  }
  m_profileChoice->setCurrentIndex(activeIndex);
  m_updatingProfiles = false;

  if (!activeProfileId.isEmpty() && activeIndex == 0)
  {
    appendError("invalid_response", "The active profile was not present in the list.");
    stopRuntime("invalid_response");
    return;
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
