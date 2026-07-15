/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/ProfileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace tb::architect
{
namespace
{

constexpr auto ProfileSchema = "architect-profile/1";
constexpr auto ActiveProfileSchema = "architect-active-profile/1";
constexpr auto ProfileFilename = "profile.json";
constexpr auto DesignLanguageFilename = "design-language.md";
constexpr auto ActiveProfileFilename = "active-profile.json";
constexpr qsizetype MaximumProfileFileBytes = 64 * 1024;
constexpr qsizetype MaximumDesignLanguageBytes = 8 * 1024;

Error profileError(const ErrorCode code, std::string message)
{
  return Error{code, std::move(message)};
}

QString qString(const std::string& value)
{
  return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string stdString(const QString& value)
{
  return value.toUtf8().toStdString();
}

QString makeSlug(const QString& value)
{
  const auto normalized = value.normalized(QString::NormalizationForm_KD).toCaseFolded();
  auto result = QString{};
  auto separatorPending = false;
  for (const auto character : normalized)
  {
    if (character.unicode() < 128 && character.isLetterOrNumber())
    {
      if (separatorPending && !result.isEmpty())
      {
        result += '-';
      }
      result += character;
      separatorPending = false;
    }
    else
    {
      separatorPending = !result.isEmpty();
    }
  }
  return result;
}

QJsonObject profileToJson(const ArchitecturalProfile& profile)
{
  auto aliases = QJsonArray{};
  for (const auto& alias : profile.aliases)
  {
    aliases.push_back(qString(alias));
  }
  return QJsonObject{
    {"schema", ProfileSchema},
    {"id", qString(profile.id)},
    {"display_name", qString(profile.displayName)},
    {"slug", qString(profile.slug)},
    {"aliases", std::move(aliases)},
    {"version", profile.version},
    {"status", qString(profile.status)},
  };
}

ProfileResult profileFromJson(const QJsonObject& json, const QString& directorySlug)
{
  if (json.value("schema").toString() != ProfileSchema)
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The profile schema is unsupported.");
  }

  const auto id = json.value("id").toString();
  const auto displayName = json.value("display_name").toString();
  const auto slug = json.value("slug").toString();
  const auto versionValue = json.value("version");
  const auto version = versionValue.toInt(0);
  const auto status = json.value("status").toString();
  if (
    QUuid{id}.isNull() || displayName.isEmpty() || displayName.size() > 128
    || displayName != displayName.trimmed() || !versionValue.isDouble()
    || versionValue.toDouble() != static_cast<double>(version) || slug != directorySlug
    || slug != makeSlug(displayName) || version < 1 || status != "draft")
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The profile metadata is invalid.");
  }

  auto aliases = std::vector<std::string>{};
  const auto aliasesJson = json.value("aliases");
  if (!aliasesJson.isArray() || aliasesJson.toArray().size() > 32)
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The profile aliases are invalid.");
  }
  for (const auto& value : aliasesJson.toArray())
  {
    const auto alias = value.toString();
    if (
      !value.isString() || alias.isEmpty() || alias.size() > 128
      || alias != alias.trimmed())
    {
      return profileError(
        ErrorCode::UnsupportedProfileSchema, "The profile aliases are invalid.");
    }
    aliases.push_back(stdString(alias));
  }

  return ArchitecturalProfile{
    .id = stdString(id),
    .displayName = stdString(displayName),
    .slug = stdString(slug),
    .aliases = std::move(aliases),
    .version = version,
    .status = stdString(status),
  };
}

std::optional<Error> writeJson(QSaveFile& file, const QJsonObject& json)
{
  if (!file.open(QIODevice::WriteOnly))
  {
    return profileError(
      ErrorCode::InternalError, "The profile file could not be opened.");
  }
  const auto bytes = QJsonDocument{json}.toJson(QJsonDocument::Indented);
  if (file.write(bytes) != bytes.size() || !file.commit())
  {
    return profileError(ErrorCode::InternalError, "The profile file could not be saved.");
  }
  return std::nullopt;
}

std::variant<QJsonObject, Error> readJson(const QString& path)
{
  auto file = QFile{path};
  if (
    !file.open(QIODevice::ReadOnly) || file.size() < 1
    || file.size() > MaximumProfileFileBytes)
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The profile file is invalid.");
  }
  const auto document = QJsonDocument::fromJson(file.readAll());
  if (!document.isObject())
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The profile file is invalid.");
  }
  return document.object();
}

bool containedBy(const QString& canonicalRoot, const QString& canonicalPath)
{
  if (canonicalRoot.isEmpty() || canonicalPath.isEmpty())
  {
    return false;
  }
  const auto normalizedRoot = QDir::fromNativeSeparators(canonicalRoot);
  const auto normalizedPath = QDir::fromNativeSeparators(canonicalPath);
#if defined(Q_OS_WIN)
  return normalizedPath.startsWith(normalizedRoot + '/', Qt::CaseInsensitive);
#else
  return normalizedPath.startsWith(normalizedRoot + '/');
#endif
}

ProfileResult uniqueMatch(std::vector<ArchitecturalProfile> matches)
{
  if (matches.empty())
  {
    return profileError(ErrorCode::ProfileNotFound, "The profile was not found.");
  }
  if (matches.size() > 1)
  {
    return profileError(
      ErrorCode::AmbiguousProfileName, "The profile name matches multiple profiles.");
  }
  return std::move(matches.front());
}

} // namespace

ProfileStore::ProfileStore(QString rootPath)
  : m_rootPath{QDir::cleanPath(std::move(rootPath))}
{
}

const QString& ProfileStore::rootPath() const
{
  return m_rootPath;
}

ProfileListResult ProfileStore::list() const
{
  auto root = QDir{m_rootPath};
  if (!root.exists())
  {
    return std::vector<ArchitecturalProfile>{};
  }

  const auto canonicalRoot = QFileInfo{root.absolutePath()}.canonicalFilePath();
  if (canonicalRoot.isEmpty())
  {
    return profileError(ErrorCode::InternalError, "The profile store is unavailable.");
  }

  auto profiles = std::vector<ArchitecturalProfile>{};
  const auto directories = root.entryInfoList(
    QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::NoSymLinks, QDir::Name);
  profiles.reserve(static_cast<std::size_t>(directories.size()));
  for (const auto& directory : directories)
  {
    const auto metadataPath =
      QDir{directory.absoluteFilePath()}.filePath(ProfileFilename);
    const auto metadataInfo = QFileInfo{metadataPath};
    if (
      !metadataInfo.isFile()
      || !containedBy(canonicalRoot, metadataInfo.canonicalFilePath()))
    {
      return profileError(
        ErrorCode::UnsupportedProfileSchema, "A profile path is invalid.");
    }

    const auto json = readJson(metadataPath);
    if (const auto* error = std::get_if<Error>(&json))
    {
      return *error;
    }
    auto profile = profileFromJson(std::get<QJsonObject>(json), directory.fileName());
    if (const auto* error = std::get_if<Error>(&profile))
    {
      return *error;
    }
    profiles.push_back(std::get<ArchitecturalProfile>(std::move(profile)));
  }
  return profiles;
}

ProfileResult ProfileStore::createDraft(
  std::string displayName, std::string designLanguage, std::vector<std::string> aliases)
{
  const auto displayNameString = qString(displayName).trimmed();
  const auto designLanguageString = qString(designLanguage).trimmed();
  const auto slug = makeSlug(displayNameString);
  if (
    displayNameString.isEmpty() || displayNameString.size() > 128 || slug.isEmpty()
    || designLanguageString.isEmpty()
    || designLanguageString.toUtf8().size() > MaximumDesignLanguageBytes
    || aliases.size() > 32)
  {
    return profileError(
      ErrorCode::InvalidArgument, "The profile name or design language is invalid.");
  }
  auto normalizedAliases = std::vector<std::string>{};
  normalizedAliases.reserve(aliases.size());
  for (const auto& alias : aliases)
  {
    const auto value = qString(alias).trimmed();
    if (value.isEmpty() || value.size() > 128)
    {
      return profileError(ErrorCode::InvalidArgument, "A profile alias is invalid.");
    }
    normalizedAliases.push_back(stdString(value));
  }

  auto root = QDir{m_rootPath};
  if (!root.mkpath("."))
  {
    return profileError(
      ErrorCode::InternalError, "The profile store could not be created.");
  }
  if (root.exists(slug) || !root.mkdir(slug))
  {
    return profileError(
      ErrorCode::InvalidArgument, "A profile with this normalized name already exists.");
  }

  const auto profileDirectoryPath = root.filePath(slug);
  auto cleanup = [&]() { QDir{profileDirectoryPath}.removeRecursively(); };
  const auto canonicalRoot = QFileInfo{root.absolutePath()}.canonicalFilePath();
  const auto profileDirectory = QFileInfo{profileDirectoryPath};
  if (
    canonicalRoot.isEmpty() || !profileDirectory.isDir() || profileDirectory.isSymLink()
    || !containedBy(canonicalRoot, profileDirectory.canonicalFilePath()))
  {
    cleanup();
    return profileError(ErrorCode::InternalError, "The profile path is invalid.");
  }
  auto profile = ArchitecturalProfile{
    .id = stdString(QUuid::createUuid().toString(QUuid::WithoutBraces)),
    .displayName = stdString(displayNameString),
    .slug = stdString(slug),
    .aliases = std::move(normalizedAliases),
    .version = 1,
    .status = "draft",
  };

  auto designLanguageFile =
    QSaveFile{QDir{profileDirectoryPath}.filePath(DesignLanguageFilename)};
  if (!designLanguageFile.open(QIODevice::WriteOnly))
  {
    cleanup();
    return profileError(
      ErrorCode::InternalError, "The profile design language could not be opened.");
  }
  const auto markdown =
    QString{"# %1 design language\n\nStatus: Draft\nVersion: 1\n\n%2\n"}
      .arg(displayNameString, designLanguageString)
      .toUtf8();
  if (
    designLanguageFile.write(markdown) != markdown.size() || !designLanguageFile.commit())
  {
    cleanup();
    return profileError(
      ErrorCode::InternalError, "The profile design language could not be saved.");
  }

  auto metadataFile = QSaveFile{QDir{profileDirectoryPath}.filePath(ProfileFilename)};
  if (const auto error = writeJson(metadataFile, profileToJson(profile)))
  {
    cleanup();
    return *error;
  }
  return profile;
}

ProfileResult ProfileStore::resolve(const std::string& reference) const
{
  const auto profilesResult = list();
  if (const auto* error = std::get_if<Error>(&profilesResult))
  {
    return *error;
  }
  const auto& profiles = std::get<std::vector<ArchitecturalProfile>>(profilesResult);
  const auto referenceString = qString(reference).trimmed();

  auto displayMatches = std::vector<ArchitecturalProfile>{};
  for (const auto& profile : profiles)
  {
    if (qString(profile.displayName).compare(referenceString, Qt::CaseInsensitive) == 0)
    {
      displayMatches.push_back(profile);
    }
  }
  if (!displayMatches.empty())
  {
    return uniqueMatch(std::move(displayMatches));
  }

  auto aliasMatches = std::vector<ArchitecturalProfile>{};
  for (const auto& profile : profiles)
  {
    if (std::ranges::any_of(profile.aliases, [&](const auto& alias) {
          return qString(alias).compare(referenceString, Qt::CaseInsensitive) == 0;
        }))
    {
      aliasMatches.push_back(profile);
    }
  }
  if (!aliasMatches.empty())
  {
    return uniqueMatch(std::move(aliasMatches));
  }

  const auto referenceSlug = makeSlug(referenceString);
  auto slugMatches = std::vector<ArchitecturalProfile>{};
  for (const auto& profile : profiles)
  {
    if (qString(profile.slug) == referenceSlug)
    {
      slugMatches.push_back(profile);
    }
  }
  return uniqueMatch(std::move(slugMatches));
}

ActiveProfileResult ProfileStore::active() const
{
  const auto activePath = QDir{m_rootPath}.filePath(ActiveProfileFilename);
  const auto activeInfo = QFileInfo{activePath};
  if (!activeInfo.exists())
  {
    return std::optional<ArchitecturalProfile>{};
  }
  const auto canonicalRoot =
    QFileInfo{QDir{m_rootPath}.absolutePath()}.canonicalFilePath();
  if (!activeInfo.isFile() || !containedBy(canonicalRoot, activeInfo.canonicalFilePath()))
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The active profile path is invalid.");
  }
  const auto jsonResult = readJson(activePath);
  if (const auto* error = std::get_if<Error>(&jsonResult))
  {
    return *error;
  }
  const auto json = std::get<QJsonObject>(jsonResult);
  const auto id = json.value("profile_id").toString();
  if (json.value("schema").toString() != ActiveProfileSchema || QUuid{id}.isNull())
  {
    return profileError(
      ErrorCode::UnsupportedProfileSchema, "The active profile metadata is invalid.");
  }

  const auto profilesResult = list();
  if (const auto* error = std::get_if<Error>(&profilesResult))
  {
    return *error;
  }
  for (const auto& profile : std::get<std::vector<ArchitecturalProfile>>(profilesResult))
  {
    if (qString(profile.id) == id)
    {
      return std::optional<ArchitecturalProfile>{profile};
    }
  }
  return profileError(ErrorCode::ProfileNotFound, "The active profile was not found.");
}

ProfileResult ProfileStore::setActive(const std::string& reference)
{
  auto profileResult = resolve(reference);
  if (const auto* error = std::get_if<Error>(&profileResult))
  {
    return *error;
  }
  const auto profile = std::get<ArchitecturalProfile>(std::move(profileResult));
  auto file = QSaveFile{QDir{m_rootPath}.filePath(ActiveProfileFilename)};
  if (
    const auto error = writeJson(
      file,
      QJsonObject{
        {"schema", ActiveProfileSchema},
        {"profile_id", qString(profile.id)},
      }))
  {
    return *error;
  }
  return profile;
}

std::optional<Error> ProfileStore::clearActive()
{
  const auto path = QDir{m_rootPath}.filePath(ActiveProfileFilename);
  if (QFileInfo::exists(path) && !QFile::remove(path))
  {
    return profileError(
      ErrorCode::InternalError, "The active profile could not be cleared.");
  }
  return std::nullopt;
}

} // namespace tb::architect
