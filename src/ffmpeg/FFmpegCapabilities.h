#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

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
    bool isScanning() const;

    // Starts a capability scan off the GUI thread. Returns false only when
    // there is no usable FFmpeg to scan. A request issued while a scan is in
    // flight is coalesced into a single follow-up run.
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

    enum class HwVendor {
        None,
        Nvidia,
        Intel,
        Amd,
        Apple,
        Vaapi
    };
    Q_ENUM(HwVendor)

    struct HwDevice {
        HwVendor vendor = HwVendor::None;
        QString name;
        QString hwAccelMethod;
        QStringList supportedEncoders;
    };

    QVector<HwDevice> detectedHwDevices() const;
    bool hasHwAcceleration() const;
    QString primaryHwDeviceName() const;

    // Returns best operational hardware encoder for a given video codec (e.g. "h264" -> "h264_nvenc")
    QString bestHwEncoder(const QString &baseCodec) const;

    // Returns recommended decode -hwaccel method for an encoder (e.g. "h264_nvenc" -> "cuda")
    QString recommendedHwAccelForEncoder(const QString &encoder) const;

    // Parses `ffmpeg -formats/-encoders/-decoders/-filters` listing output
    // (or the plain `ffmpeg -hwaccels` list) into a name set.
    static QSet<QString> parseListOutput(const QString &output);

    // Encoder names worth offering in the UI. They still have to be filtered
    // through filterEncoders() because not every build ships every encoder.
    static QStringList videoEncoderCandidates();
    static QStringList audioEncoderCandidates();

    // Subset of candidates this build can encode with. Before the first
    // scan() the candidates pass through unchanged so the UI still has
    // something to offer.
    QStringList filterEncoders(const QStringList &candidates) const;

signals:
    void capabilitiesChanged();

private:
    // Everything a scan produces, computed on a worker thread so the GUI
    // thread never blocks on the FFmpeg subprocesses.
    struct ScanResult {
        QSet<QString> formats;
        QSet<QString> codecs;
        QSet<QString> encoders;
        QSet<QString> decoders;
        QSet<QString> filters;
        QSet<QString> hwAccels;
        QSet<QString> verifiedHwEncoders;
        QVector<HwDevice> hwDevices;
        bool loaded = false;
    };

    static ScanResult runScan(const QString &ffmpeg);
    void launchScan(const QString &ffmpeg);
    void onScanFinished();

    const FFmpegLocator *m_locator;
    QSet<QString> m_formats;
    QSet<QString> m_codecs;
    QSet<QString> m_encoders;
    QSet<QString> m_decoders;
    QSet<QString> m_filters;
    QSet<QString> m_hwAccels;
    QSet<QString> m_verifiedHwEncoders;
    QVector<HwDevice> m_hwDevices;
    bool m_loaded = false;

    QFutureWatcher<ScanResult> *m_watcher = nullptr;
    bool m_scanRunning = false;
    bool m_rescanQueued = false;
};
