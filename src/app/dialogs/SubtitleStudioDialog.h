#pragma once

#include <QDialog>
#include <QList>
#include <QProcess>
#include <QString>

#include "subtitles/SubtitleEngine.h"
#include "subtitles/WhisperPipeline.h"

class JobQueue;
class FFmpegLocator;
class VideoPlayerWidget;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaSpinBox;
class ElaComboBox;
class ElaProgressBar;
class QTableWidget;
class QLabel;

class SubtitleStudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubtitleStudioDialog(JobQueue *queue, FFmpegLocator *locator,
                                  QWidget *parent = nullptr);
    ~SubtitleStudioDialog() override;

    void setVideoFile(const QString &videoPath);
    void retranslateUi();

private slots:
    void onPlayerPositionChanged(qint64 positionMs);
    void onTableRowSelected(int row, int column);
    void onTableCellChanged(int row, int column);

    void importSubtitle();
    void exportSubtitle();
    void runAiSpeechRecognition();
    void applyTimeShift();
    void addRow();
    void removeSelectedRows();
    void mergeSelectedRows();

    void burnSubtitleToVideo();
    void softMuxSubtitleToVideo();

private:
    void rebuildTable();
    void updateOverlayText(qint64 currentMs);
    void syncItemsFromTable();

    JobQueue *m_queue = nullptr;
    FFmpegLocator *m_locator = nullptr;

    QString m_videoPath;
    QList<SubtitleItem> m_items;
    bool m_isUpdatingTable = false;

    // Header
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;

    // Video Player & Overlay
    VideoPlayerWidget *m_player = nullptr;
    QLabel *m_overlayBanner = nullptr;

    // Toolbar
    ElaPushButton *m_importBtn = nullptr;
    ElaPushButton *m_exportBtn = nullptr;
    ElaPushButton *m_aiWhisperBtn = nullptr;
    ElaComboBox *m_aiLangCombo = nullptr;
    ElaSpinBox *m_shiftSpin = nullptr;
    ElaPushButton *m_shiftBtn = nullptr;
    ElaPushButton *m_addRowBtn = nullptr;
    ElaPushButton *m_delRowBtn = nullptr;
    ElaPushButton *m_mergeBtn = nullptr;

    // Table
    QTableWidget *m_table = nullptr;

    // AI Status / Progress
    ElaProgressBar *m_aiProgressBar = nullptr;
    ElaText *m_aiStatusLabel = nullptr;
    QProcess *m_aiProcess = nullptr;

    // Bottom Action Buttons
    ElaPushButton *m_burnBtn = nullptr;
    ElaPushButton *m_softMuxBtn = nullptr;
    ElaPushButton *m_closeBtn = nullptr;
};
