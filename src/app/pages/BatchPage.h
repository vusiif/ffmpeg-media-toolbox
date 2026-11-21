#pragma once

#include <QWidget>

class FFmpegLocator;
class FFmpegCommandBuilder;
class FFmpegCapabilities;
class JobQueue;
class PresetManager;

class ElaText;
class ElaPushButton;
class BatchFileListCard;
class BatchOptionsCard;
class BatchPipelineCard;
class BatchMatrixCard;

class BatchPage : public QWidget
{
    Q_OBJECT

public:
    explicit BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       FFmpegCapabilities *caps, JobQueue *queue,
                       QWidget *parent = nullptr);
    ~BatchPage() override = default;

    void addFiles(const QStringList &files);
    void addFolder(const QString &folder);
    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void addAllToQueue();
    void onWatcherFileReady(const QString &filePath, const QString &relativeSubDir);

private:
    FFmpegLocator *m_locator = nullptr;
    FFmpegCommandBuilder *m_builder = nullptr;
    FFmpegCapabilities *m_caps = nullptr;
    JobQueue *m_queue = nullptr;
    PresetManager *m_presets = nullptr;

    ElaText *m_titleLabel = nullptr;
    BatchFileListCard *m_fileCard = nullptr;
    BatchOptionsCard *m_optionsCard = nullptr;
    BatchPipelineCard *m_pipelineCard = nullptr;
    BatchMatrixCard *m_matrixCard = nullptr;
    ElaPushButton *m_queueButton = nullptr;
    bool m_isDragHover = false;
};
