#pragma once

#include <QObject>
#include <QString>
#include <QSet>

class FFmpegLocator;

class FFmpegCapabilities : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY capabilitiesChanged)
    Q_PROPERTY(int formatCount READ formatCount NOTIFY capabilitiesChanged)
    Q_PROPERTY(int encoderCount READ encoderCount NOTIFY capabilitiesChanged)
    Q_PROPERTY(int decoderCount READ decoderCount NOTIFY capabilitiesChanged)
    Q_PROPERTY(int filterCount READ filterCount NOTIFY capabilitiesChanged)
    Q_PROPERTY(int hwAccelCount READ hwAccelCount NOTIFY capabilitiesChanged)

public:
    explicit FFmpegCapabilities(const FFmpegLocator *locator, QObject *parent = nullptr);

    bool isLoaded() const;

    Q_INVOKABLE bool scan();

    bool hasFormat(const QString &name) const;
    bool hasCodec(const QString &name) const;
    bool hasEncoder(const QString &name) const;
    bool hasDecoder(const QString &name) const;
    bool hasFilter(const QString &name) const;
    bool hasHwAccel(const QString &name) const;

    QSet<QString> formats() const;
    QSet<QString> codecs() const;
    QSet<QString> encoders() const;
    QSet<QString> decoders() const;
    QSet<QString> filters() const;
    QSet<QString> hwAccels() const;

    int formatCount() const;
    int encoderCount() const;
    int decoderCount() const;
    int filterCount() const;
    int hwAccelCount() const;

signals:
    void capabilitiesChanged();

private:
    QSet<QString> parseListOutput(const QString &output) const;

    const FFmpegLocator *m_locator;
    QSet<QString> m_formats;
    QSet<QString> m_codecs;
    QSet<QString> m_encoders;
    QSet<QString> m_decoders;
    QSet<QString> m_filters;
    QSet<QString> m_hwAccels;
    bool m_loaded = false;
};
