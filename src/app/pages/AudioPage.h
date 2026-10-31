#pragma once

#include <QWidget>

class FFmpegLocator;
class FFmpegCommandBuilder;
class JobQueue;
class QStackedWidget;
class ElaPivot;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaCheckBox;

class AudioPage : public QWidget
{
    Q_OBJECT

public:
    explicit AudioPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       JobQueue *queue, QWidget *parent = nullptr);

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString browseFile(const QString &title, const QString &filters);
    void enqueue(const QString &name, const QStringList &args);

    void processFormat();
    void processDenoise();
    void processLoudness();
    void processTempo();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    JobQueue *m_queue;

    QString m_inputFile;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;

    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_formatTabTitle;
    QString m_denoiseTabTitle;
    QString m_loudnessTabTitle;
    QString m_tempoTabTitle;

    // Tab 1: 格式与提取 (Format & Transcode)
    ElaScrollPageArea *m_formatCard = nullptr;
    ElaText *m_formatCardTitle = nullptr;
    ElaText *m_outFormatCaption = nullptr;
    ElaComboBox *m_outFormatCombo = nullptr;
    ElaText *m_bitrateCaption = nullptr;
    ElaComboBox *m_bitrateCombo = nullptr;
    ElaText *m_samplerateCaption = nullptr;
    ElaComboBox *m_samplerateCombo = nullptr;
    ElaText *m_channelsCaption = nullptr;
    ElaComboBox *m_channelsCombo = nullptr;
    ElaPushButton *m_formatButton = nullptr;

    // Tab 2: 降噪与滤波 (Denoise & Filter)
    ElaScrollPageArea *m_denoiseCard = nullptr;
    ElaText *m_denoiseCardTitle = nullptr;
    ElaCheckBox *m_denoiseCheck = nullptr;
    ElaText *m_denoiseLevelCaption = nullptr;
    ElaSpinBox *m_denoiseLevelSpin = nullptr;
    ElaCheckBox *m_highpassCheck = nullptr;
    ElaText *m_highpassCaption = nullptr;
    ElaSpinBox *m_highpassSpin = nullptr;
    ElaCheckBox *m_lowpassCheck = nullptr;
    ElaText *m_lowpassCaption = nullptr;
    ElaSpinBox *m_lowpassSpin = nullptr;
    ElaPushButton *m_denoiseButton = nullptr;

    // Tab 3: 响度与静音 (Loudness & Silence)
    ElaScrollPageArea *m_loudnessCard = nullptr;
    ElaText *m_loudnessCardTitle = nullptr;
    ElaCheckBox *m_loudnormCheck = nullptr;
    ElaCheckBox *m_silenceCheck = nullptr;
    ElaText *m_volumeCaption = nullptr;
    ElaDoubleSpinBox *m_volumeSpin = nullptr;
    ElaPushButton *m_loudnessButton = nullptr;

    // Tab 4: 变速与效果 (Tempo & Effects)
    ElaScrollPageArea *m_tempoCard = nullptr;
    ElaText *m_tempoCardTitle = nullptr;
    ElaText *m_tempoCaption = nullptr;
    ElaDoubleSpinBox *m_tempoSpin = nullptr;
    ElaPushButton *m_tempo075Btn = nullptr;
    ElaPushButton *m_tempo100Btn = nullptr;
    ElaPushButton *m_tempo125Btn = nullptr;
    ElaPushButton *m_tempo150Btn = nullptr;
    ElaPushButton *m_tempo200Btn = nullptr;
    ElaCheckBox *m_reverseCheck = nullptr;
    ElaPushButton *m_tempoButton = nullptr;
};
