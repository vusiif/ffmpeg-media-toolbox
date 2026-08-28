#include "Application.h"

#include <QUrl>
#include <QQuickStyle>

Application::Application(QObject *parent)
    : QObject(parent)
{
}

Application::~Application() = default;

int Application::run(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    m_app = &app;

    setupEngine();

    if (m_engine->rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}

void Application::setupEngine()
{
    m_engine = new QQmlApplicationEngine(this);

    const QUrl url(QStringLiteral("qrc:/qt/qml/ffmedia/qml/Main.qml"));

    QObject::connect(
        m_engine,
        &QQmlApplicationEngine::objectCreationFailed,
        m_app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    m_engine->load(url);
}
