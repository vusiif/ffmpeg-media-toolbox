#pragma once

#include <QFutureWatcher>
#include <QString>
#include <QVariantMap>

#include "ElaScrollPageArea.h"
#include "media/MediaFile.h"

class MediaProbeService;
class RecentFiles;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaComboBox;

class ConvertSourceCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit ConvertSourceCard(MediaProbeService *probe, RecentFiles *recentFiles, QWidget *parent = nullptr);
    ~ConvertSourceCard() override = default;

    QString inputFile() const;
    void setInputFile(const QString &path);
    void startProbe(bool enqueueAfter);
    const MediaFile &probeInfo() const { return m_probeInfo; }

    void refreshRecentList();
    void showMetadata(const QVariantMap &tags);
    void retranslateUi();

signals:
    void inputFileChanged(const QString &path);
    void probeFinished(const MediaFile &info, bool enqueueAfter);

public slots:
    void browseInput();

private slots:
    void onProbeWatcherFinished();

private:
    void loadRecentFile(const QString &path);

    MediaProbeService *m_probe = nullptr;
    RecentFiles *m_recentFiles = nullptr;
    QFutureWatcher<MediaFile> *m_probeWatcher = nullptr;

    QString m_inputFile;
    MediaFile m_probeInfo;
    bool m_enqueueAfterProbe = false;

    ElaText *m_inputCardTitle = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    ElaText *m_recentCaption = nullptr;
    ElaComboBox *m_recentCombo = nullptr;
    ElaText *m_infoLabel = nullptr;
    ElaText *m_metaCaption = nullptr;
    ElaText *m_metaValue = nullptr;
};
