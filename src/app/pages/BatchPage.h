#pragma once

#include <QStringList>
#include <QWidget>

class FFmpegLocator;
class FFmpegCommandBuilder;
class FFmpegCapabilities;
class JobQueue;
class QListWidget;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaSpinBox;
class ElaCheckBox;

class BatchPage : public QWidget
{
    Q_OBJECT

public:
    explicit BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       FFmpegCapabilities *caps, JobQueue *queue,
                       QWidget *parent = nullptr);

    void retranslateUi();

private:
    void addFiles();
    void clearFiles();
    void browseOutputDir();
    void addAllToQueue();
    void rebuildCodecList();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    FFmpegCapabilities *m_caps;
    JobQueue *m_queue;

    QStringList m_inputFiles;
    QString m_outputDirectory;

    ElaText *m_titleLabel = nullptr;
    QListWidget *m_fileList = nullptr;
    ElaPushButton *m_addButton = nullptr;
    ElaPushButton *m_clearButton = nullptr;
    ElaText *m_countLabel = nullptr;
    ElaText *m_outputDirCaption = nullptr;
    ElaPushButton *m_outputDirButton = nullptr;
    ElaText *m_formatCaption = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaText *m_codecCaption = nullptr;
    ElaComboBox *m_codecCombo = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaPushButton *m_queueButton = nullptr;
};
