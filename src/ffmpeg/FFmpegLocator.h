#pragma once

#include <QObject>
#include <QString>
#include <QSettings>

class FFmpegLocator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath NOTIFY pathsChanged)
    Q_PROPERTY(QString ffprobePath READ ffprobePath NOTIFY pathsChanged)
    Q_PROPERTY(QString ffmpegVersion READ ffmpegVersion NOTIFY pathsChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY pathsChanged)

public:
    explicit FFmpegLocator(QObject *parent = nullptr);

    QString ffmpegPath() const;
    QString ffprobePath() const;
    QString ffmpegVersion() const;
    bool isValid() const;

    Q_INVOKABLE bool autoDetect();
    Q_INVOKABLE bool setCustomPaths(const QString &ffmpeg, const QString &ffprobe);
    Q_INVOKABLE void resetToAutoDetect();

signals:
    void pathsChanged();

private:
    struct ProbeResult {
        QString path;
        QString version;
    };

    ProbeResult probe(const QString &candidate) const;
    QString findInPath(const QString &executable) const;
    QString findCommonLocations(const QString &executable) const;
    void loadSavedPaths();
    void savePaths() const;

    QString m_ffmpegPath;
    QString m_ffprobePath;
    QString m_ffmpegVersion;
    bool m_customPaths = false;
};
