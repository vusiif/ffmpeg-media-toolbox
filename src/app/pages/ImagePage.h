#pragma once

#include <QFutureWatcher>
#include <QPixmap>
#include <QWidget>

class FFmpegLocator;
class FFmpegCommandBuilder;
class JobQueue;
class QLabel;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaSpinBox;
class ElaCheckBox;

class ImagePage : public QWidget
{
    Q_OBJECT

public:
    explicit ImagePage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       JobQueue *queue, QWidget *parent = nullptr);

    void retranslateUi();

private:
    void browseInput();
    void startPreview();
    void addToQueue();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    JobQueue *m_queue;

    QString m_inputFile;
    QFutureWatcher<QPixmap> *m_previewWatcher = nullptr;

    ElaText *m_titleLabel = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    QLabel *m_previewLabel = nullptr;
    ElaText *m_fileLabel = nullptr;
    ElaText *m_formatCaption = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaText *m_cropCaption = nullptr;
    ElaSpinBox *m_cropX = nullptr;
    ElaSpinBox *m_cropY = nullptr;
    ElaSpinBox *m_cropW = nullptr;
    ElaSpinBox *m_cropH = nullptr;
    ElaText *m_resizeCaption = nullptr;
    ElaSpinBox *m_resizeW = nullptr;
    ElaSpinBox *m_resizeH = nullptr;
    ElaText *m_rotateCaption = nullptr;
    ElaComboBox *m_rotateCombo = nullptr;
    ElaText *m_flipCaption = nullptr;
    ElaCheckBox *m_flipH = nullptr;
    ElaCheckBox *m_flipV = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaPushButton *m_addButton = nullptr;
};
