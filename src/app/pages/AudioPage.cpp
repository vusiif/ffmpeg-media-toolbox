#include "AudioPage.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPivot.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

AudioPage::AudioPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    // Common Audio/Video Source Card
    auto *inputCard = new ElaScrollPageArea(this);
    inputCard->setMinimumHeight(0);
    inputCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *inputCardLayout = new QHBoxLayout(inputCard);
    inputCardLayout->setContentsMargins(18, 12, 18, 12);
    inputCardLayout->setSpacing(12);

    m_inputCaption = new ElaText(QString(), 13, inputCard);
    m_inputEdit = new ElaLineEdit(inputCard);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(inputCard);
    connect(m_browseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(
            trId("nav.audio"),
            QStringLiteral("Audio/Video files (*.mp3 *.m4a *.aac *.flac *.wav *.ogg *.opus *.mp4 *.mkv *.mov *.webm);;All files (*)"));
        if (!path.isEmpty()) {
            m_inputFile = path;
            m_inputEdit->setText(path);
        }
    });
    inputCardLayout->addWidget(m_inputCaption);
    inputCardLayout->addWidget(m_inputEdit, 1);
    inputCardLayout->addWidget(m_browseButton);
    root->addWidget(inputCard);

    // Segmented Navigation Switcher
    m_pivot = new ElaPivot(this);
    m_pivot->setPivotSpacing(16);
    m_pivot->setMarkWidth(90);
    root->addWidget(m_pivot);

    m_stackedWidget = new QStackedWidget(this);
    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    root->addWidget(m_stackedWidget, 1);

    auto createTab = [this](QWidget *&bodyWidget, QVBoxLayout *&bodyLayout) {
        auto *tab = new QWidget(m_stackedWidget);
        auto *tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 4, 0, 0);
        tabLayout->setSpacing(0);

        auto *scroll = new ElaScrollArea(tab);
        scroll->setWidgetResizable(true);
        scroll->setIsGrabGesture(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        bodyWidget = new QWidget(scroll);
        bodyWidget->setStyleSheet(QStringLiteral("background-color: transparent;"));
        bodyLayout = new QVBoxLayout(bodyWidget);
        bodyLayout->setContentsMargins(0, 0, 10, 0);
        bodyLayout->setSpacing(14);

        scroll->setWidget(bodyWidget);
        tabLayout->addWidget(scroll);
        return tab;
    };

    auto createCard = [](QWidget *parent, ElaText *&titleLabel) {
        auto *card = new ElaScrollPageArea(parent);
        card->setMinimumHeight(0);
        card->setMaximumHeight(QWIDGETSIZE_MAX);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(10);

        titleLabel = new ElaText(QString(), 15, card);
        titleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        layout->addWidget(titleLabel);
        return card;
    };

    auto createCardButton = [](ElaScrollPageArea *card) {
        auto *button = new ElaPushButton(card);
        button->setFixedHeight(36);
        button->setMinimumWidth(140);
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        btnRow->addWidget(button);
        if (auto *layout = qobject_cast<QVBoxLayout *>(card->layout())) {
            layout->addLayout(btnRow);
        }
        return button;
    };

    auto makeCaption = [](QWidget *parent) {
        auto *caption = new ElaText(QString(), 13, parent);
        caption->setMinimumWidth(130);
        return caption;
    };

    // =========================================================================
    // Tab 1: 格式与提取 (Format & Transcode)
    // =========================================================================
    QWidget *bodyFormat = nullptr;
    QVBoxLayout *bodyFormatLayout = nullptr;
    QWidget *tabFormat = createTab(bodyFormat, bodyFormatLayout);

    m_formatCard = createCard(bodyFormat, m_formatCardTitle);
    auto *fmtForm = new QFormLayout();
    fmtForm->setSpacing(10);

    m_outFormatCaption = makeCaption(m_formatCard);
    m_outFormatCombo = new ElaComboBox(m_formatCard);
    m_outFormatCombo->addItems({QStringLiteral("mp3"), QStringLiteral("aac"),
                                QStringLiteral("flac"), QStringLiteral("wav"),
                                QStringLiteral("opus"), QStringLiteral("ogg"),
                                QStringLiteral("m4a")});

    m_bitrateCaption = makeCaption(m_formatCard);
    m_bitrateCombo = new ElaComboBox(m_formatCard);
    m_bitrateCombo->addItem(QStringLiteral("128 kbps"), 128);
    m_bitrateCombo->addItem(QStringLiteral("192 kbps"), 192);
    m_bitrateCombo->addItem(QStringLiteral("256 kbps"), 256);
    m_bitrateCombo->addItem(QStringLiteral("320 kbps (高品质 / High)"), 320);
    m_bitrateCombo->setCurrentIndex(1); // 192 kbps

    m_samplerateCaption = makeCaption(m_formatCard);
    m_samplerateCombo = new ElaComboBox(m_formatCard);
    m_samplerateCombo->addItem(QStringLiteral("Auto (保持原样)"), -1);
    m_samplerateCombo->addItem(QStringLiteral("44100 Hz (CD 标准)"), 44100);
    m_samplerateCombo->addItem(QStringLiteral("48000 Hz (影视标准)"), 48000);
    m_samplerateCombo->addItem(QStringLiteral("96000 Hz (高解析度)"), 96000);

    m_channelsCaption = makeCaption(m_formatCard);
    m_channelsCombo = new ElaComboBox(m_formatCard);
    m_channelsCombo->addItem(QStringLiteral("Auto (保持原样)"), -1);
    m_channelsCombo->addItem(QStringLiteral("单声道 (Mono)"), 1);
    m_channelsCombo->addItem(QStringLiteral("立体声 (Stereo)"), 2);
    m_channelsCombo->addItem(QStringLiteral("5.1 环绕声 (5.1 Surround)"), 6);

    fmtForm->addRow(m_outFormatCaption, m_outFormatCombo);
    fmtForm->addRow(m_bitrateCaption, m_bitrateCombo);
    fmtForm->addRow(m_samplerateCaption, m_samplerateCombo);
    fmtForm->addRow(m_channelsCaption, m_channelsCombo);
    qobject_cast<QVBoxLayout *>(m_formatCard->layout())->addLayout(fmtForm);

    m_formatButton = createCardButton(m_formatCard);
    connect(m_formatButton, &ElaPushButton::clicked, this, &AudioPage::processFormat);
    bodyFormatLayout->addWidget(m_formatCard);
    bodyFormatLayout->addStretch();
    m_stackedWidget->addWidget(tabFormat);

    // =========================================================================
    // Tab 2: 降噪与滤波 (Denoise & Filter)
    // =========================================================================
    QWidget *bodyDenoise = nullptr;
    QVBoxLayout *bodyDenoiseLayout = nullptr;
    QWidget *tabDenoise = createTab(bodyDenoise, bodyDenoiseLayout);

    m_denoiseCard = createCard(bodyDenoise, m_denoiseCardTitle);
    auto *dnForm = new QFormLayout();
    dnForm->setSpacing(10);

    m_denoiseCheck = new ElaCheckBox(m_denoiseCard);
    m_denoiseCheck->setChecked(true);
    m_denoiseLevelCaption = makeCaption(m_denoiseCard);
    m_denoiseLevelSpin = new ElaSpinBox(m_denoiseCard);
    m_denoiseLevelSpin->setRange(6, 25);
    m_denoiseLevelSpin->setValue(12);
    m_denoiseLevelSpin->setSuffix(QStringLiteral(" dB"));

    m_highpassCheck = new ElaCheckBox(m_denoiseCard);
    m_highpassCaption = makeCaption(m_denoiseCard);
    m_highpassSpin = new ElaSpinBox(m_denoiseCard);
    m_highpassSpin->setRange(20, 500);
    m_highpassSpin->setValue(80);
    m_highpassSpin->setSuffix(QStringLiteral(" Hz"));

    m_lowpassCheck = new ElaCheckBox(m_denoiseCard);
    m_lowpassCaption = makeCaption(m_denoiseCard);
    m_lowpassSpin = new ElaSpinBox(m_denoiseCard);
    m_lowpassSpin->setRange(3000, 20000);
    m_lowpassSpin->setValue(12000);
    m_lowpassSpin->setSuffix(QStringLiteral(" Hz"));

    dnForm->addRow(m_denoiseCheck);
    dnForm->addRow(m_denoiseLevelCaption, m_denoiseLevelSpin);
    dnForm->addRow(m_highpassCheck);
    dnForm->addRow(m_highpassCaption, m_highpassSpin);
    dnForm->addRow(m_lowpassCheck);
    dnForm->addRow(m_lowpassCaption, m_lowpassSpin);
    qobject_cast<QVBoxLayout *>(m_denoiseCard->layout())->addLayout(dnForm);

    m_denoiseButton = createCardButton(m_denoiseCard);
    connect(m_denoiseButton, &ElaPushButton::clicked, this, &AudioPage::processDenoise);
    bodyDenoiseLayout->addWidget(m_denoiseCard);
    bodyDenoiseLayout->addStretch();
    m_stackedWidget->addWidget(tabDenoise);

    // =========================================================================
    // Tab 3: 响度与静音 (Loudness & Silence)
    // =========================================================================
    QWidget *bodyLoudness = nullptr;
    QVBoxLayout *bodyLoudnessLayout = nullptr;
    QWidget *tabLoudness = createTab(bodyLoudness, bodyLoudnessLayout);

    m_loudnessCard = createCard(bodyLoudness, m_loudnessCardTitle);
    auto *ldForm = new QFormLayout();
    ldForm->setSpacing(10);

    m_loudnormCheck = new ElaCheckBox(m_loudnessCard);
    m_loudnormCheck->setChecked(true);

    m_silenceCheck = new ElaCheckBox(m_loudnessCard);

    m_volumeCaption = makeCaption(m_loudnessCard);
    m_volumeSpin = new ElaDoubleSpinBox(m_loudnessCard);
    m_volumeSpin->setRange(0.1, 5.0);
    m_volumeSpin->setSingleStep(0.1);
    m_volumeSpin->setValue(1.0);
    m_volumeSpin->setSuffix(QStringLiteral(" x"));

    ldForm->addRow(m_loudnormCheck);
    ldForm->addRow(m_silenceCheck);
    ldForm->addRow(m_volumeCaption, m_volumeSpin);
    qobject_cast<QVBoxLayout *>(m_loudnessCard->layout())->addLayout(ldForm);

    m_loudnessButton = createCardButton(m_loudnessCard);
    connect(m_loudnessButton, &ElaPushButton::clicked, this, &AudioPage::processLoudness);
    bodyLoudnessLayout->addWidget(m_loudnessCard);
    bodyLoudnessLayout->addStretch();
    m_stackedWidget->addWidget(tabLoudness);

    // =========================================================================
    // Tab 4: 变速与效果 (Tempo & Effects)
    // =========================================================================
    QWidget *bodyTempo = nullptr;
    QVBoxLayout *bodyTempoLayout = nullptr;
    QWidget *tabTempo = createTab(bodyTempo, bodyTempoLayout);

    m_tempoCard = createCard(bodyTempo, m_tempoCardTitle);
    auto *tpForm = new QFormLayout();
    tpForm->setSpacing(10);

    m_tempoCaption = makeCaption(m_tempoCard);
    auto *tempoRow = new QHBoxLayout();
    m_tempoSpin = new ElaDoubleSpinBox(m_tempoCard);
    m_tempoSpin->setRange(0.5, 2.0);
    m_tempoSpin->setSingleStep(0.05);
    m_tempoSpin->setValue(1.0);
    m_tempoSpin->setSuffix(QStringLiteral(" x"));

    m_tempo075Btn = new ElaPushButton(QStringLiteral("0.75x"), m_tempoCard);
    m_tempo100Btn = new ElaPushButton(QStringLiteral("1.0x"), m_tempoCard);
    m_tempo125Btn = new ElaPushButton(QStringLiteral("1.25x"), m_tempoCard);
    m_tempo150Btn = new ElaPushButton(QStringLiteral("1.5x"), m_tempoCard);
    m_tempo200Btn = new ElaPushButton(QStringLiteral("2.0x"), m_tempoCard);
    for (ElaPushButton *btn : {m_tempo075Btn, m_tempo100Btn, m_tempo125Btn, m_tempo150Btn, m_tempo200Btn}) {
        btn->setMaximumWidth(55);
    }
    connect(m_tempo075Btn, &ElaPushButton::clicked, this, [this]() { m_tempoSpin->setValue(0.75); });
    connect(m_tempo100Btn, &ElaPushButton::clicked, this, [this]() { m_tempoSpin->setValue(1.0); });
    connect(m_tempo125Btn, &ElaPushButton::clicked, this, [this]() { m_tempoSpin->setValue(1.25); });
    connect(m_tempo150Btn, &ElaPushButton::clicked, this, [this]() { m_tempoSpin->setValue(1.5); });
    connect(m_tempo200Btn, &ElaPushButton::clicked, this, [this]() { m_tempoSpin->setValue(2.0); });

    tempoRow->addWidget(m_tempoSpin, 1);
    tempoRow->addWidget(m_tempo075Btn);
    tempoRow->addWidget(m_tempo100Btn);
    tempoRow->addWidget(m_tempo125Btn);
    tempoRow->addWidget(m_tempo150Btn);
    tempoRow->addWidget(m_tempo200Btn);

    m_reverseCheck = new ElaCheckBox(m_tempoCard);

    tpForm->addRow(m_tempoCaption, tempoRow);
    tpForm->addRow(m_reverseCheck);
    qobject_cast<QVBoxLayout *>(m_tempoCard->layout())->addLayout(tpForm);

    m_tempoButton = createCardButton(m_tempoCard);
    connect(m_tempoButton, &ElaPushButton::clicked, this, &AudioPage::processTempo);
    bodyTempoLayout->addWidget(m_tempoCard);
    bodyTempoLayout->addStretch();
    m_stackedWidget->addWidget(tabTempo);

    retranslateUi();
}

void AudioPage::retranslateUi()
{
    const int currentIdx = m_pivot->getCurrentIndex();
    for (const QString &title : {m_formatTabTitle, m_denoiseTabTitle, m_loudnessTabTitle, m_tempoTabTitle}) {
        if (!title.isEmpty()) {
            m_pivot->removePivot(title);
        }
    }

    m_formatTabTitle = trId("audio.tab.format");
    m_denoiseTabTitle = trId("audio.tab.denoise");
    m_loudnessTabTitle = trId("audio.tab.loudness");
    m_tempoTabTitle = trId("audio.tab.tempo");

    m_pivot->appendPivot(m_formatTabTitle);
    m_pivot->appendPivot(m_denoiseTabTitle);
    m_pivot->appendPivot(m_loudnessTabTitle);
    m_pivot->appendPivot(m_tempoTabTitle);
    m_pivot->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);

    m_titleLabel->setText(trId("audio.title"));
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));

    // Tab 1
    m_formatCardTitle->setText(trId("audio.card.format"));
    m_outFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_bitrateCaption->setText(trId("convert.adv.audioBitrate") + QStringLiteral(":"));
    m_samplerateCaption->setText(trId("convert.adv.samplerate") + QStringLiteral(":"));
    m_channelsCaption->setText(trId("convert.adv.channels") + QStringLiteral(":"));
    m_formatButton->setText(trId("convert.addToQueue"));

    // Tab 2
    m_denoiseCardTitle->setText(trId("audio.card.denoise"));
    m_denoiseCheck->setText(trId("audio.denoise.enable"));
    m_denoiseLevelCaption->setText(trId("audio.denoise.strength") + QStringLiteral(":"));
    m_highpassCheck->setText(trId("audio.highpass.enable"));
    m_highpassCaption->setText(trId("audio.highpass.freq") + QStringLiteral(":"));
    m_lowpassCheck->setText(trId("audio.lowpass.enable"));
    m_lowpassCaption->setText(trId("audio.lowpass.freq") + QStringLiteral(":"));
    m_denoiseButton->setText(trId("convert.addToQueue"));

    // Tab 3
    m_loudnessCardTitle->setText(trId("audio.card.loudness"));
    m_loudnormCheck->setText(trId("audio.loudnorm.enable"));
    m_silenceCheck->setText(trId("audio.silence.enable"));
    m_volumeCaption->setText(trId("audio.volume.gain") + QStringLiteral(":"));
    m_loudnessButton->setText(trId("convert.addToQueue"));

    // Tab 4
    m_tempoCardTitle->setText(trId("audio.card.tempo"));
    m_tempoCaption->setText(trId("audio.tempo.speed") + QStringLiteral(":"));
    m_reverseCheck->setText(trId("audio.reverse.enable"));
    m_tempoButton->setText(trId("convert.addToQueue"));
}

QString AudioPage::browseFile(const QString &title, const QString &filters)
{
    return QFileDialog::getOpenFileName(this, title, QString(), filters);
}

void AudioPage::enqueue(const QString &name, const QStringList &args)
{
    if (args.isEmpty()) return;
    auto *job = new Job();
    job->setName(name);
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void AudioPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void AudioPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        const QString path = urls.first().toLocalFile();
        if (!path.isEmpty()) {
            m_inputFile = path;
            m_inputEdit->setText(path);
        }
    }
}

void AudioPage::processFormat()
{
    if (m_inputFile.isEmpty()) return;

    const QString fmt = m_outFormatCombo->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_audio.{format}"),
                                               fmt);

    FFmpegCommandBuilder::AudioProcessOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = output;
    opts.format = fmt;
    opts.bitrateKbps = m_bitrateCombo->currentData().toInt();
    opts.sampleRate = m_samplerateCombo->currentData().toInt();
    opts.channels = m_channelsCombo->currentData().toInt();

    const QString jobName = trId("audio.job.format").arg(QFileInfo(m_inputFile).fileName(), fmt);
    enqueue(jobName, m_builder->buildAudioProcess(opts));
}

void AudioPage::processDenoise()
{
    if (m_inputFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix().toLower();
    const QString fmt = (suffix.isEmpty() || suffix == QLatin1String("mp4") || suffix == QLatin1String("mkv") || suffix == QLatin1String("mov"))
                            ? QStringLiteral("mp3")
                            : suffix;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_denoised.{format}"),
                                               fmt);

    FFmpegCommandBuilder::AudioProcessOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = output;
    opts.format = fmt;
    opts.denoise = m_denoiseCheck->isChecked();
    opts.denoiseNoiseReduction = m_denoiseLevelSpin->value();
    if (m_highpassCheck->isChecked()) {
        opts.highpassFreq = m_highpassSpin->value();
    }
    if (m_lowpassCheck->isChecked()) {
        opts.lowpassFreq = m_lowpassSpin->value();
    }

    const QString jobName = trId("audio.job.denoise").arg(QFileInfo(m_inputFile).fileName());
    enqueue(jobName, m_builder->buildAudioProcess(opts));
}

void AudioPage::processLoudness()
{
    if (m_inputFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix().toLower();
    const QString fmt = (suffix.isEmpty() || suffix == QLatin1String("mp4") || suffix == QLatin1String("mkv") || suffix == QLatin1String("mov"))
                            ? QStringLiteral("mp3")
                            : suffix;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_loudness.{format}"),
                                               fmt);

    FFmpegCommandBuilder::AudioProcessOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = output;
    opts.format = fmt;
    opts.loudnorm = m_loudnormCheck->isChecked();
    opts.removeSilence = m_silenceCheck->isChecked();
    opts.volumeFactor = m_volumeSpin->value();

    const QString jobName = trId("audio.job.loudness").arg(QFileInfo(m_inputFile).fileName());
    enqueue(jobName, m_builder->buildAudioProcess(opts));
}

void AudioPage::processTempo()
{
    if (m_inputFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix().toLower();
    const QString fmt = (suffix.isEmpty() || suffix == QLatin1String("mp4") || suffix == QLatin1String("mkv") || suffix == QLatin1String("mov"))
                            ? QStringLiteral("mp3")
                            : suffix;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_tempo.{format}"),
                                               fmt);

    FFmpegCommandBuilder::AudioProcessOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = output;
    opts.format = fmt;
    opts.tempo = m_tempoSpin->value();
    opts.reverse = m_reverseCheck->isChecked();

    const QString jobName = trId("audio.job.tempo").arg(QFileInfo(m_inputFile).fileName());
    enqueue(jobName, m_builder->buildAudioProcess(opts));
}
