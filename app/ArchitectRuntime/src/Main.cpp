/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileDevice>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonParseError>

#include "architect/Error.h"
#include "architect/ProfileStore.h"
#include "architect/Protocol.h"

#include <cstdio>
#include <optional>
#include <utility>

namespace
{

void writeResponse(QFile& output, const QJsonObject& response)
{
  output.write(QJsonDocument{response}.toJson(QJsonDocument::Compact));
  output.write("\n");
  output.flush();
}

void discardLineRemainder(QFile& input, QByteArray line)
{
  while (!line.endsWith('\n') && !input.atEnd())
  {
    line = input.readLine(4096);
  }
}

} // namespace

int main(int argc, char* argv[])
{
  auto app = QCoreApplication{argc, argv};
  QCoreApplication::setApplicationName("TrenchBroomArchitectRuntime");

  auto profileStore = std::optional<tb::architect::ProfileStore>{};
  const auto arguments = QCoreApplication::arguments();
  if (arguments.size() == 3 && arguments[1] == "--profile-root")
  {
    if (arguments[2].trimmed().isEmpty())
    {
      return 2;
    }
    profileStore.emplace(arguments[2]);
  }
  else if (arguments.size() != 1)
  {
    return 2;
  }

  auto input = QFile{};
  auto output = QFile{};
  if (
    !input.open(stdin, QIODevice::ReadOnly, QFileDevice::DontCloseHandle)
    || !output.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle))
  {
    return 2;
  }

  while (true)
  {
    auto line =
      input.readLine(static_cast<qint64>(tb::architect::MaximumRequestBytes) + 2);
    if (line.isEmpty() && input.atEnd())
    {
      break;
    }
    if (line.trimmed().isEmpty())
    {
      continue;
    }
    if (line.size() > static_cast<qsizetype>(tb::architect::MaximumRequestBytes))
    {
      discardLineRemainder(input, std::move(line));
      writeResponse(
        output,
        tb::architect::makeErrorResponse(
          {},
          tb::architect::Error{
            tb::architect::ErrorCode::InvalidRequest,
            "The request exceeds 65536 bytes.",
          }));
      continue;
    }

    auto parseError = QJsonParseError{};
    const auto document = QJsonDocument::fromJson(line, &parseError);
    if (
      parseError.error != QJsonParseError::NoError || document.isNull()
      || !document.isObject())
    {
      writeResponse(
        output,
        tb::architect::makeErrorResponse(
          {},
          tb::architect::Error{
            tb::architect::ErrorCode::InvalidRequest,
            "The request must be one JSON object on one line.",
          }));
      continue;
    }

    writeResponse(
      output,
      tb::architect::handleRequest(
        document.object(), profileStore ? &*profileStore : nullptr));
  }

  return 0;
}
