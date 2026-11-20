#include "VideoAudioTab.h"

#include <cmath>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>

#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "i18n/TrId.h"
#include "output/OutputNaming.h"

VideoAudioTab::VideoAudioTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                             JobQueue *queue, MediaProbeService *probeService,
                             QWidget *parent)
    : VideoTabBase(locator, builder, queue, probeService, parent)
{
    initAudioCard();
    initMuteCard();
    initBgmCard();
    initSyncCard();
    m_bodyLayout->addStretch();
}

void VideoAudioTab::initAudioCard()
{
    m_audioCard = createCard(m_audioTitleLabel);
    auto *audioForm = new QFormLayout();
    audioForm->setSpacing(8);
    m_audioFormatCaption = makeCaption(m_audioCard);
    m_audioFormat = new ElaComboBox(m_audioCard);
    m_audioFormat->addItems({QStringLiteral("mp3"), QStringLiteral("aac"), QStringLiteral("flac"),
                             QStringLiteral("wav"), QStringLiteral("opus"), QStringLiteral("m4a")});
    m_audioBitrateCaption = makeCaption(m_audioCard);
    m_audioBitrate = new ElaSpinBox(m_audioCard);
    m_audioBitrate->setRange(0, 512);
    m_audioBitrate->setValue(192);
    audioForm->addRow(m_audioFormatCaption, m_audioFormat);
    audioForm->addRow(m_audioBitrateCaption, m_audioBitrate);
    qobject_cast<QVBoxLayout *>(m_audioCard->layout())->addLayout(audioForm);
    m_audioButton = createCardButton(m_audioCard);
    connect(m_audioButton, &ElaPushButton::clicked, this, &VideoAudioTab::extractAudio);
    m_bodyLayout->addWidget(m_audioCard);
}

void VideoAudioTab::initMuteCard()
{
    m_muteCard = createCard(m_muteTitleLabel);
    auto *muteForm = new QFormLayout();
    muteForm->setSpacing(8);
    m_muteFormatCaption = makeCaption(m_muteCard);
    m_muteFormat = new ElaComboBox(m_muteCard);
    m_muteFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("webm")});
    muteForm->addRow(m_muteFormatCaption, m_muteFormat);
    qobject_cast<QVBoxLayout *>(m_muteCard->layout())->addLayout(muteForm);
    m_muteButton = createCardButton(m_muteCard);
    connect(m_muteButton, &ElaPushButton::clicked, this, &VideoAudioTab::mute);
    m_bodyLayout->addWidget(m_muteCard);
}

void VideoAudioTab::initBgmCard()
{
    m_bgmCard = createCard(m_bgmTitleLabel);
    auto *bgmForm = new QFormLayout();
    bgmForm->setSpacing(8);

    auto *bgmPathRow = new QHBoxLayout();
    m_bgmPath = new ElaLineEdit(m_bgmCard);
    m_bgmPath->setReadOnly(true);
    m_bgmBrowseButton = new ElaPushButton(m_bgmCard);
    connect(m_bgmBrowseButton, &ElaPushButton::clicked, this, [this]() {
        const QString path = browseFile(trId("video.card.bgm"),
                                        QStringLiteral("Audio files (*.mp3 *.wav *.aac *.m4a *.flac *.ogg);;All files (*)"));
        if (!path.isEmpty()) {
            m_bgmPath->setText(path);
        }
    });
    bgmPathRow->addWidget(m_bgmPath, 1);
    bgmPathRow->addWidget(m_bgmBrowseButton);

    m_bgmPathCaption = makeCaption(m_bgmCard);
    m_bgmModeCaption = makeCaption(m_bgmCard);
    m_bgmMode = new ElaComboBox(m_bgmCard);
    m_bgmMode->addItems({QString(), QString()});

    m_bgmOrigVolCaption = makeCaption(m_bgmCard);
    m_bgmOrigVol = new ElaSpinBox(m_bgmCard);
    m_bgmOrigVol->setRange(0, 300);
    m_bgmOrigVol->setValue(100);
    m_bgmOrigVol->setSuffix(QStringLiteral(" %"));

    m_bgmMusicVolCaption = makeCaption(m_bgmCard);
    m_bgmMusicVol = new ElaSpinBox(m_bgmCard);
    m_bgmMusicVol->setRange(0, 300);
    m_bgmMusicVol->setValue(30);
    m_bgmMusicVol->setSuffix(QStringLiteral(" %"));

    connect(m_bgmMode, QOverload<int>::of(&ElaComboBox::currentIndexChanged), this, [this](int idx) {
        const bool isMix = (idx == 1);
        m_bgmOrigVol->setEnabled(isMix);
        m_bgmMusicVol->setEnabled(isMix);
    });
    m_bgmOrigVol->setEnabled(false);
    m_bgmMusicVol->setEnabled(false);

    bgmForm->addRow(m_bgmPathCaption, bgmPathRow);
    bgmForm->addRow(m_bgmModeCaption, m_bgmMode);
    bgmForm->addRow(m_bgmOrigVolCaption, m_bgmOrigVol);
    bgmForm->addRow(m_bgmMusicVolCaption, m_bgmMusicVol);

    qobject_cast<QVBoxLayout *>(m_bgmCard->layout())->addLayout(bgmForm);
    m_bgmButton = createCardButton(m_bgmCard);
    connect(m_bgmButton, &ElaPushButton::clicked, this, &VideoAudioTab::applyBgm);
    m_bodyLayout->addWidget(m_bgmCard);
}

void VideoAudioTab::initSyncCard()
{
    m_syncCard = createCard(m_syncTitleLabel);
    auto *syncForm = new QFormLayout();
    syncForm->setSpacing(8);

    m_syncOffsetCaption = makeCaption(m_syncCard);
    m_syncOffset = new ElaDoubleSpinBox(m_syncCard);
    m_syncOffset->setRange(-10.0, 10.0);
    m_syncOffset->setSingleStep(0.05);
    m_syncOffset->setDecimals(2);
    m_syncOffset->setValue(0.0);
    m_syncOffset->setSuffix(QStringLiteral(" s"));

    m_syncOffsetTip = new ElaText(QString(), 11, m_syncCard);
    m_syncOffsetTip->setStyleSheet(QStringLiteral("color: #888888;"));

    auto *offsetRow = new QHBoxLayout();
    offsetRow->addWidget(m_syncOffset);
    offsetRow->addWidget(m_syncOffsetTip, 1);

    m_syncVolumeCaption = makeCaption(m_syncCard);
    m_syncVolume = new ElaSpinBox(m_syncCard);
    m_syncVolume->setRange(0, 400);
    m_syncVolume->setSingleStep(5);
    m_syncVolume->setValue(100);
    m_syncVolume->setSuffix(QStringLiteral(" %"));

    syncForm->addRow(m_syncOffsetCaption, offsetRow);
    syncForm->addRow(m_syncVolumeCaption, m_syncVolume);

    qobject_cast<QVBoxLayout *>(m_syncCard->layout())->addLayout(syncForm);
    m_syncButton = createCardButton(m_syncCard);
    connect(m_syncButton, &ElaPushButton::clicked, this, &VideoAudioTab::applyAudioSync);
    m_bodyLayout->addWidget(m_syncCard);
}

void VideoAudioTab::extractAudio()
{
    if (m_inputFile.isEmpty()) return;
    const QString format = m_audioFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}.{format}"), format);
    enqueue(trId("video.job.audio").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildExtractAudio(m_inputFile, output, format, QString(),
                                         m_audioBitrate->value()));
}

void VideoAudioTab::mute()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_muteFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_muteFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_muted.{format}"), ext);
    enqueue(trId("video.job.mute").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildMute(m_inputFile, output, m_muteFormat->currentText(),
                                 QString(), -1, true));
}

void VideoAudioTab::applyBgm()
{
    if (m_inputFile.isEmpty()) return;
    const QString bgmFile = m_bgmPath->text().trimmed();
    if (bgmFile.isEmpty() || !QFile::exists(bgmFile)) {
        return;
    }

    const QString formatExt = QFileInfo(m_inputFile).suffix().isEmpty() ? QStringLiteral("mp4") : QFileInfo(m_inputFile).suffix();
    const QString outName = OutputNaming::build(m_inputFile, QStringLiteral("{name}_bgm.{format}"), formatExt);

    const int mode = m_bgmMode->currentIndex();
    QStringList args;
    if (mode == 0) {
        // Mode 0: Replace original audio with BGM
        args << QStringLiteral("-i") << m_inputFile
             << QStringLiteral("-i") << bgmFile
             << QStringLiteral("-map") << QStringLiteral("0:v:0")
             << QStringLiteral("-map") << QStringLiteral("1:a:0")
             << QStringLiteral("-c:v") << QStringLiteral("copy")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-shortest")
             << outName;
    } else {
        // Mode 1: Mix BGM with original audio
        const double v1 = m_bgmOrigVol->value() / 100.0;
        const double v2 = m_bgmMusicVol->value() / 100.0;
        const QString filter = QStringLiteral("[0:a]volume=%1[a0];[1:a]volume=%2[a1];[a0][a1]amix=inputs=2:duration=first:dropout_transition=2[aout]")
                                   .arg(v1, 0, 'f', 2)
                                   .arg(v2, 0, 'f', 2);
        args << QStringLiteral("-i") << m_inputFile
             << QStringLiteral("-i") << bgmFile
             << QStringLiteral("-filter_complex") << filter
             << QStringLiteral("-map") << QStringLiteral("0:v:0")
             << QStringLiteral("-map") << QStringLiteral("[aout]")
             << QStringLiteral("-c:v") << QStringLiteral("copy")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << outName;
    }

    enqueue(trId("video.bgm.addQueue") + QStringLiteral(" - ") + QFileInfo(m_inputFile).fileName(), args);
}

void VideoAudioTab::applyAudioSync()
{
    if (m_inputFile.isEmpty()) return;
    const double offsetSec = m_syncOffset->value();
    const int volPercent = m_syncVolume->value();
    const double volFactor = volPercent / 100.0;

    const QString formatExt = QFileInfo(m_inputFile).suffix().isEmpty() ? QStringLiteral("mp4") : QFileInfo(m_inputFile).suffix();
    const QString outName = OutputNaming::build(m_inputFile, QStringLiteral("{name}_synced.{format}"), formatExt);

    QStringList filters;
    if (std::abs(volFactor - 1.0) > 0.01) {
        filters << QStringLiteral("volume=%1").arg(volFactor, 0, 'f', 2);
    }
    if (offsetSec > 0.001) {
        const int ms = static_cast<int>(std::round(offsetSec * 1000.0));
        filters << QStringLiteral("adelay=%1|%1").arg(ms);
    } else if (offsetSec < -0.001) {
        const double adv = -offsetSec;
        filters << QStringLiteral("atrim=start=%1,asetpts=PTS-STARTPTS").arg(adv, 0, 'f', 3);
    }

    QStringList args;
    args << QStringLiteral("-i") << m_inputFile
         << QStringLiteral("-map") << QStringLiteral("0:v:0")
         << QStringLiteral("-map") << QStringLiteral("0:a?")
         << QStringLiteral("-c:v") << QStringLiteral("copy");

    if (filters.isEmpty()) {
        args << QStringLiteral("-c:a") << QStringLiteral("copy");
    } else {
        args << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-af") << filters.join(QLatin1Char(','));
    }
    args << outName;

    enqueue(trId("video.sync.addQueue") + QStringLiteral(" - ") + QFileInfo(m_inputFile).fileName(), args);
}

void VideoAudioTab::retranslateUi()
{
    m_audioTitleLabel->setText(trId("video.audio"));
    m_audioFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_audioBitrateCaption->setText(trId("audio.bitrate") + QStringLiteral(":"));
    m_audioButton->setText(trId("convert.addToQueue"));

    m_muteTitleLabel->setText(trId("video.mute"));
    m_muteFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_muteButton->setText(trId("convert.addToQueue"));

    m_bgmTitleLabel->setText(trId("video.card.bgm"));
    m_bgmPathCaption->setText(trId("video.bgm.musicPath"));
    m_bgmBrowseButton->setText(trId("common.browse"));
    m_bgmModeCaption->setText(trId("video.bgm.mode"));
    m_bgmMode->setItemText(0, trId("video.bgm.modeReplace"));
    m_bgmMode->setItemText(1, trId("video.bgm.modeMix"));
    m_bgmOrigVolCaption->setText(trId("video.bgm.origVol"));
    m_bgmMusicVolCaption->setText(trId("video.bgm.musicVol"));
    m_bgmButton->setText(trId("video.bgm.addQueue"));

    m_syncTitleLabel->setText(trId("video.card.sync"));
    m_syncOffsetCaption->setText(trId("video.sync.offset"));
    m_syncOffsetTip->setText(trId("video.sync.offsetTip"));
    m_syncVolumeCaption->setText(trId("video.sync.volume"));
    m_syncButton->setText(trId("video.sync.addQueue"));
}
