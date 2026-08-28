#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QGuiApplication>

class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    int run(int argc, char *argv[]);

private:
    void setupEngine();

    QGuiApplication *m_app = nullptr;
    QQmlApplicationEngine *m_engine = nullptr;
};
