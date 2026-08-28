#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQml/qqml.h>

#include "ffmpeg/FFmpegLocator.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "media/MediaProbeService.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "i18n/LanguageManager.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("FFMedia"));
    app.setApplicationName(QStringLiteral("FFmpeg Media Toolbox"));

    qmlRegisterType<Job>("ffmedia", 1, 0, "Job");
    qmlRegisterType<FFmpegCommandBuilder>("ffmedia", 1, 0, "FFmpegCommandBuilder");

    FFmpegLocator ffmpegLocator;
    FFmpegCapabilities ffmpegCaps(&ffmpegLocator);
    FFmpegCommandBuilder commandBuilder;
    MediaProbeService mediaProbe(&ffmpegLocator);
    JobQueue jobQueue(&ffmpegLocator, &mediaProbe);
    LanguageManager languageManager;

    QQmlApplicationEngine engine;

    languageManager.setQmlEngine(&engine);

    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegLocator"), &ffmpegLocator);
    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegCaps"), &ffmpegCaps);
    engine.rootContext()->setContextProperty(QStringLiteral("commandBuilder"), &commandBuilder);
    engine.rootContext()->setContextProperty(QStringLiteral("mediaProbe"), &mediaProbe);
    engine.rootContext()->setContextProperty(QStringLiteral("jobQueue"), &jobQueue);
    engine.rootContext()->setContextProperty(QStringLiteral("jobModel"), jobQueue.model());
    engine.rootContext()->setContextProperty(QStringLiteral("languageManager"), &languageManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.loadFromModule("ffmedia", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
