#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include <filesystem>
#include <iostream>

#include "rating_bridge.hpp"

namespace {
bool require(bool value, const char* message) {
    if (!value)
        std::cerr << "rating_bridge_test: " << message << '\n';
    return value;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // Act as a deterministic local shell transport when the packaged probe
    // starts us with the script as its first argument. No hf or network is used.
    if (argc > 1 && QFileInfo{QString::fromLocal8Bit(argv[1])}.fileName() ==
                        QStringLiteral("sync_ratings.sh")) {
        QFile marker{qEnvironmentVariable("DIAMOND_RATING_TEST_MARKER")};
        if (!marker.open(QIODevice::WriteOnly))
            return 1;
        return marker.write(QByteArray{argv[1]}) >= 0 ? 0 : 1;
    }
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--packaged-sync")) {
        RatingBridge packaged{std::filesystem::path(argv[2])};
        QObject::connect(&packaged, &RatingBridge::changed, &app, [&] {
            if (packaged.syncState() == QStringLiteral("Ratings synced."))
                app.quit();
        });
        packaged.syncPending();
        if (!require(packaged.syncRunning(), "packaged launcher missing from unrelated cwd"))
            return 1;
        QTimer::singleShot(10000, &app, [&app] { app.exit(1); });
        return app.exec();
    }
    qputenv("DIAMOND_RATING_SYNC_DISABLED", QByteArrayLiteral("1"));
    QTemporaryDir temporary;
    if (!require(temporary.isValid(), "temporary root unavailable"))
        return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    QCoreApplication::setOrganizationName(QStringLiteral("AlphaDiamondTest"));
    QCoreApplication::setApplicationName(QStringLiteral("RatingBridge"));

    const QString model = QDir(temporary.path()).filePath(QStringLiteral("model"));
    QDir().mkpath(model);
    QFile metadata{QDir(model).filePath(QStringLiteral("metadata.json"))};
    if (!require(metadata.open(QIODevice::WriteOnly), "metadata unavailable"))
        return 1;
    metadata.write(QJsonDocument(QJsonObject{{"model_sha256", QString(64, QLatin1Char('b'))},
                                             {"runtime_sha256", QString(64, QLatin1Char('a'))}})
                       .toJson());
    metadata.close();

    RatingBridge bridge{
        std::filesystem::path(temporary.filePath(QStringLiteral("ratings")).toStdString())};
    const QVariantList order{1, 2};
    const bool published = bridge.recordTerminalSooMatch(QStringLiteral("local-game-1"), model,
                                                         QStringLiteral("Soo/2.0.0"),
                                                         QStringLiteral("Soo v2.0.0"), 2, 1, order);
    if (!require(published, "terminal model game did not publish a durable event")) {
        std::cerr << bridge.syncState().toStdString() << '\n';
        return 1;
    }
    if (!require(!bridge.recordTerminalSooMatch(QStringLiteral("local-game-1"), model,
                                                QStringLiteral("Soo/2.0.0"),
                                                QStringLiteral("Soo v2.0.0"), 2, 1, order),
                 "same terminal game was not idempotent"))
        return 1;
    if (!require(bridge.pendingEventCount() == 1, "exactly one event was not retained for retry"))
        return 1;
    if (!require(!bridge.syncRunning() &&
                     bridge.syncState().contains(QStringLiteral("queued"), Qt::CaseInsensitive),
                 "missing sync launcher did not leave durable retry state"))
        return 1;
    qunsetenv("DIAMOND_RATING_SYNC_DISABLED");
    qputenv("DIAMOND_RATING_SYNC_BINARY",
            temporary.filePath(QStringLiteral("missing-rating-sync")).toUtf8());
    bridge.syncPending();
    if (!require(!bridge.syncRunning() && bridge.pendingEventCount() == 1 &&
                     bridge.syncState().contains(QStringLiteral("launcher"), Qt::CaseInsensitive),
                 "missing sync binary did not preserve the pending event"))
        return 1;
    const QString package = temporary.filePath(QStringLiteral("package"));
    const QString unrelated = temporary.filePath(QStringLiteral("unrelated"));
    if (!require(QDir().mkpath(package + QStringLiteral("/tools")) && QDir().mkpath(unrelated),
                 "packaged probe directories unavailable"))
        return 1;
    const QString executable =
        QDir(package).filePath(QFileInfo{app.applicationFilePath()}.fileName());
    if (!require(QFile::copy(app.applicationFilePath(), executable),
                 "probe executable copy failed"))
        return 1;
    const QString script = QDir(package).filePath(QStringLiteral("tools/sync_ratings.sh"));
    QFile launcher{script};
    if (!require(launcher.open(QIODevice::WriteOnly), "probe launcher unavailable"))
        return 1;
    launcher.write("# Deterministic local transport fixture; the test executable acts as shell.\n");
    launcher.close();
    const QString marker = temporary.filePath(QStringLiteral("launcher-selected"));
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("DIAMOND_RATING_SYNC_DISABLED"));
    environment.remove(QStringLiteral("DIAMOND_RATING_SYNC_SCRIPT"));
    environment.insert(QStringLiteral("DIAMOND_RATING_SYNC_SHELL"), executable);
    environment.insert(QStringLiteral("DIAMOND_RATING_SYNC_BINARY"), executable);
    environment.insert(QStringLiteral("DIAMOND_RATING_TEST_MARKER"), marker);
    QProcess probe;
    probe.setProcessEnvironment(environment);
    probe.setWorkingDirectory(unrelated);
    probe.start(executable,
                {QStringLiteral("--packaged-sync"), temporary.filePath(QStringLiteral("ratings"))});
    if (!require(probe.waitForFinished(15000) && probe.exitStatus() == QProcess::NormalExit &&
                     probe.exitCode() == 0,
                 "packaged sync failed from unrelated cwd")) {
        std::cerr << probe.readAllStandardError().toStdString();
        return 1;
    }
    QFile selected{marker};
    if (!require(selected.open(QIODevice::ReadOnly) &&
                     QString::fromUtf8(selected.readAll()) == script,
                 "packaged tools launcher was not selected"))
        return 1;
    return 0;
}
