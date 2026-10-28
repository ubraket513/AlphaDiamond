#pragma once

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <filesystem>
#include <system_error>

// Create an isolated package with the SDK fixture and a matching default index.
inline bool prepareSdkFixture(const QString& root, const QString& fixture) {
    QFile metadata(QDir(fixture).filePath(QStringLiteral("metadata.json")));
    if (!metadata.open(QIODevice::ReadOnly)) return false;
    const QJsonObject model = QJsonDocument::fromJson(metadata.readAll()).object();
    const QString family = model.value(QStringLiteral("model_family")).toString();
    const QString version = model.value(QStringLiteral("model_version")).toString();
    if (family != QStringLiteral("soo") || version != QStringLiteral("2.0.0")) return false;
    const QString relative = family + QLatin1Char('/') + version;
    const QString models = QDir(root).filePath(QStringLiteral("models"));
    if (!QDir().mkpath(QDir(models).filePath(family))) return false;
    std::error_code error;
    std::filesystem::copy(fixture.toStdString(), QDir(models).filePath(relative).toStdString(),
                          std::filesystem::copy_options::recursive, error);
    if (error) return false;
    const QJsonObject entry{{QStringLiteral("family"), family},
                            {QStringLiteral("version"), version},
                            {QStringLiteral("path"), relative},
                            {QStringLiteral("model_sha256"), model.value(QStringLiteral("model_sha256"))},
                            {QStringLiteral("runtime_sha256"), model.value(QStringLiteral("runtime_sha256"))}};
    const QJsonObject index{{QStringLiteral("index_version"), 1},
                            {QStringLiteral("models"), QJsonArray{entry}},
                            {QStringLiteral("defaults"), QJsonObject{{family, relative}}}};
    QFile output(QDir(models).filePath(QStringLiteral("index.json")));
    return output.open(QIODevice::WriteOnly) && output.write(QJsonDocument(index).toJson()) > 0;
}
