#include "VideoClipTab.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/dialogs/LosslessCutDialog.h"
#include "app/widgets/VideoPlayerWidget.h"
#include "cut/SmartCutEngine.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "output/OutputNaming.h"

VideoClipTab::VideoClipTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                           JobQueue *queue, MediaProbeService *probeService,
                           QWidget *parent)
    : VideoTabBase(locator, builder, queue, probeService, parent)
{
    initTrimCard();
    initAdjustCard();
    initJoinCard();
    m_bodyLayout->addStretch();
}

VideoClipTab::~VideoClipTab()
{
    if (m_probeProcess && m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }
}

void VideoClipTab::initTrimCard()
{
    m_trimCard = createCard(m_trimTitleLabel);
    auto *trimCardLayout = qobject_cast<QVBoxLayout *>(m_trimCard->layout());

    m_trimPlayer = new VideoPlayerWidget(m_trimCard);
    m_trimPlayer->setAutoSkipVisible(false);
    trimCardLayout->addWidget(m_trimPlayer);

    auto *playerToolbar = new QHBoxLayout();
    playerToolbar->setSpacing(8);

    m_prevKeyframeBtn = new ElaPushButton(QStringLiteral("⏮"), m_trimCard);
    m_prevKeyframeBtn->setFixedWidth(36);
    m_prevKeyframeBtn->setEnabled(false);
    m_nextKeyframeBtn = new ElaPushButton(QStringLiteral("⏭"), m_trimCard);
    m_nextKeyframeBtn->setFixedWidth(36);
    m_nextKeyframeBtn->setEnabled(false);

    m_snapInButton = new ElaPushButton(m_trimCard);
    m_snapInButton->setFixedHeight(32);
    m_snapInButton->setEnabled(false);
    m_snapOutButton = new ElaPushButton(m_trimCard);
    m_snapOutButton->setFixedHeight(32);
    m_snapOutButton->setEnabled(false);

    m_keyframeStatusLabel = new ElaText(QString(), 12, m_trimCard);
    m_openStudioButton = new ElaPushButton(m_trimCard);
    m_openStudioButton->setFixedHeight(32);

    playerToolbar->addWidget(m_prevKeyframeBtn);
    playerToolbar->addWidget(m_nextKeyframeBtn);
    playerToolbar->addWidget(m_snapInButton);
    playerToolbar->addWidget(m_snapOutButton);
    playerToolbar->addWidget(m_keyframeStatusLabel);
    playerToolbar->addStretch();
    playerToolbar->addWidget(m_openStudioButton);
    trimCardLayout->addLayout(playerToolbar);

    auto *inOutLayout = new QHBoxLayout();
    m_setInButton = new ElaPushButton(m_trimCard);
    m_setInButton->setFixedHeight(32);
    m_setOutButton = new ElaPushButton(m_trimCard);
    m_setOutButton->setFixedHeight(32);
    m_previewTrimButton = new ElaPushButton(m_trimCard);
    m_previewTrimButton->setFixedHeight(32);
    inOutLayout->addWidget(m_setInButton);
    inOutLayout->addWidget(m_setOutButton);
    inOutLayout->addWidget(m_previewTrimButton);
    inOutLayout->addStretch();
    trimCardLayout->addLayout(inOutLayout);

    auto *trimForm = new QFormLayout();
    trimForm->setSpacing(8);
    m_trimStartCaption = makeCaption(m_trimCard);
    m_trimStart = new ElaDoubleSpinBox(m_trimCard);
    m_trimStart->setRange(0, 86400);
    m_trimStart->setDecimals(1);
    m_trimEndCaption = makeCaption(m_trimCard);
    m_trimEnd = new ElaDoubleSpinBox(m_trimCard);
    m_trimEnd->setRange(0, 86400);
    m_trimEnd->setDecimals(1);
    m_trimFormatCaption = makeCaption(m_trimCard);
    m_trimFormat = new ElaComboBox(m_trimCard);
    m_trimFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("mov"), QStringLiteral("webm")});
    m_trimModeCaption = makeCaption(m_trimCard);
    m_trimMode = new ElaComboBox(m_trimCard);
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::KeyframeLossless));
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::SmartCut));
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::AccurateReencode));
    m_trimCopyCaption = makeCaption(m_trimCard);
    m_trimCopy = new ElaCheckBox(m_trimCard);
    trimForm->addRow(m_trimStartCaption, m_trimStart);
    trimForm->addRow(m_trimEndCaption, m_trimEnd);
    trimForm->addRow(m_trimModeCaption, m_trimMode);
    trimForm->addRow(m_trimFormatCaption, m_trimFormat);
    trimForm->addRow(m_trimCopyCaption, m_trimCopy);
    trimCardLayout->addLayout(trimForm);

    connect(m_setInButton, &ElaPushButton::clicked, this, [this]() {
        const double pos = m_trimPlayer->currentPositionSec();
        m_trimStart->setValue(pos);
        if (m_trimEnd->value() <= pos && m_trimPlayer->durationSec() > pos) {
            m_trimEnd->setValue(m_trimPlayer->durationSec());
        }
        m_trimPlayer->setMarkRange(m_trimStart->value(), m_trimEnd->value());
    });

    connect(m_setOutButton, &ElaPushButton::clicked, this, [this]() {
        const double pos = m_trimPlayer->currentPositionSec();
        m_trimEnd->setValue(pos);
        m_trimPlayer->setMarkRange(m_trimStart->value(), m_trimEnd->value());
    });

    connect(m_previewTrimButton, &ElaPushButton::clicked, this, [this]() {
        m_trimPlayer->seekToSec(m_trimStart->value());
        m_trimPlayer->play();
    });

    connect(m_trimStart, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_trimPlayer->setMarkRange(val, m_trimEnd->value());
        m_trimPlayer->seekToSec(val);
        updateKeyframeStatus();
    });
    connect(m_trimEnd, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_trimPlayer->setMarkRange(m_trimStart->value(), val);
        updateKeyframeStatus();
    });

    connect(m_prevKeyframeBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double cur = m_trimPlayer->currentPositionSec();
        const double target = SmartCutEngine::findPrevKeyframe(cur - 0.05, m_keyframes);
        m_trimPlayer->seekToSec(target);
    });
    connect(m_nextKeyframeBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double cur = m_trimPlayer->currentPositionSec();
        const double target = SmartCutEngine::findNextKeyframe(cur + 0.05, m_keyframes);
        m_trimPlayer->seekToSec(target);
    });
    connect(m_snapInButton, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double snapped = SmartCutEngine::findNearestKeyframe(m_trimStart->value(), m_keyframes);
        m_trimStart->setValue(snapped);
        m_trimPlayer->seekToSec(snapped);
    });
    connect(m_snapOutButton, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double snapped = SmartCutEngine::findNearestKeyframe(m_trimEnd->value(), m_keyframes);
        m_trimEnd->setValue(snapped);
    });
    connect(m_openStudioButton, &ElaPushButton::clicked, this, &VideoClipTab::openLosslessCutStudio);

    m_trimButton = createCardButton(m_trimCard);
    connect(m_trimButton, &ElaPushButton::clicked, this, &VideoClipTab::trim);

    m_bodyLayout->addWidget(m_trimCard);
}

void VideoClipTab::initAdjustCard()
{
    m_adjCard = createCard(m_adjTitleLabel);
    auto *adjForm = new QFormLayout();
    adjForm->setSpacing(8);

    m_adjCropCaption = makeCaption(m_adjCard);
    auto *cropRow = new QHBoxLayout();
    cropRow->setSpacing(6);
    m_adjCropX = new ElaSpinBox(m_adjCard);
    m_adjCropX->setRange(0, 16384);
    m_adjCropY = new ElaSpinBox(m_adjCard);
    m_adjCropY->setRange(0, 16384);
    m_adjCropW = new ElaSpinBox(m_adjCard);
    m_adjCropW->setRange(0, 16384);
    m_adjCropH = new ElaSpinBox(m_adjCard);
    m_adjCropH->setRange(0, 16384);
    m_autoCropButton = new ElaPushButton(m_adjCard);
    m_autoCropButton->setFixedHeight(32);
    m_autoCropStatus = new ElaText(QString(), 12, m_adjCard);
    connect(m_autoCropButton, &ElaPushButton::clicked, this, &VideoClipTab::detectAutoCrop);

    cropRow->addWidget(new ElaText(QStringLiteral("X:"), 12, m_adjCard));
    cropRow->addWidget(m_adjCropX);
    cropRow->addWidget(new ElaText(QStringLiteral("Y:"), 12, m_adjCard));
    cropRow->addWidget(m_adjCropY);
    cropRow->addWidget(new ElaText(QStringLiteral("W:"), 12, m_adjCard));
    cropRow->addWidget(m_adjCropW);
    cropRow->addWidget(new ElaText(QStringLiteral("H:"), 12, m_adjCard));
    cropRow->addWidget(m_adjCropH);
    cropRow->addWidget(m_autoCropButton);
    cropRow->addWidget(m_autoCropStatus);
    cropRow->addStretch();
    adjForm->addRow(m_adjCropCaption, cropRow);

    m_adjResizeCaption = makeCaption(m_adjCard);
    auto *resizeRow = new QHBoxLayout();
    resizeRow->setSpacing(6);
    m_adjResizeW = new ElaSpinBox(m_adjCard);
    m_adjResizeW->setRange(0, 16384);
    m_adjResizeH = new ElaSpinBox(m_adjCard);
    m_adjResizeH->setRange(0, 16384);
    resizeRow->addWidget(new ElaText(QStringLiteral("W:"), 12, m_adjCard));
    resizeRow->addWidget(m_adjResizeW);
    resizeRow->addWidget(new ElaText(QStringLiteral("H:"), 12, m_adjCard));
    resizeRow->addWidget(m_adjResizeH);
    resizeRow->addStretch();
    adjForm->addRow(m_adjResizeCaption, resizeRow);

    m_adjRotateCaption = makeCaption(m_adjCard);
    m_adjRotateCombo = new ElaComboBox(m_adjCard);
    m_adjRotateCombo->addItems({QStringLiteral("0"), QStringLiteral("90"), QStringLiteral("180"), QStringLiteral("270")});
    adjForm->addRow(m_adjRotateCaption, m_adjRotateCombo);

    m_adjQualityCaption = makeCaption(m_adjCard);
    m_adjQualitySpin = new ElaSpinBox(m_adjCard);
    m_adjQualitySpin->setRange(0, 51);
    m_adjQualitySpin->setValue(23);
    adjForm->addRow(m_adjQualityCaption, m_adjQualitySpin);

    qobject_cast<QVBoxLayout *>(m_adjCard->layout())->addLayout(adjForm);
    m_adjButton = createCardButton(m_adjCard);
    connect(m_adjButton, &ElaPushButton::clicked, this, &VideoClipTab::adjust);

    m_bodyLayout->addWidget(m_adjCard);
}

void VideoClipTab::initJoinCard()
{
    m_joinCard = createCard(m_joinTitleLabel);
    auto *joinCardLayout = qobject_cast<QVBoxLayout *>(m_joinCard->layout());
    m_joinList = new QListWidget(m_joinCard);
    m_joinList->setMinimumHeight(120);
    m_joinList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    joinCardLayout->addWidget(m_joinList);

    auto *joinBtns = new QHBoxLayout();
    m_joinAddButton = new ElaPushButton(m_joinCard);
    m_joinAddButton->setFixedHeight(32);
    connect(m_joinAddButton, &ElaPushButton::clicked, this, [this]() {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, trId("video.join"), QString(),
            QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        for (const QString &p : paths) {
            if (!m_joinFiles.contains(p)) {
                m_joinFiles << p;
                m_joinList->addItem(p);
            }
        }
    });

    m_joinRemoveButton = new ElaPushButton(m_joinCard);
    m_joinRemoveButton->setFixedHeight(32);
    connect(m_joinRemoveButton, &ElaPushButton::clicked, this, [this]() {
        for (auto *item : m_joinList->selectedItems()) {
            m_joinFiles.removeAll(item->text());
            delete m_joinList->takeItem(m_joinList->row(item));
        }
    });

    joinBtns->addWidget(m_joinAddButton);
    joinBtns->addWidget(m_joinRemoveButton);
    joinBtns->addStretch();
    joinCardLayout->addLayout(joinBtns);

    m_joinButton = createCardButton(m_joinCard);
    connect(m_joinButton, &ElaPushButton::clicked, this, &VideoClipTab::joinVideos);

    m_bodyLayout->addWidget(m_joinCard);
}

void VideoClipTab::setInputFile(const QString &path)
{
    VideoTabBase::setInputFile(path);
    if (m_trimPlayer && !path.isEmpty()) {
        m_trimPlayer->loadMedia(path);
    }
    detectKeyframes();
}

void VideoClipTab::detectKeyframes()
{
    if (m_inputFile.isEmpty()) return;
    if (m_probeProcess && m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }
    if (!m_probeProcess) {
        m_probeProcess = new QProcess(this);
        connect(m_probeProcess, &QProcess::finished, this, &VideoClipTab::onKeyframeProbeFinished);
    }
    const QString ffprobe = m_locator ? m_locator->ffprobePath() : QStringLiteral("ffprobe");
    const QStringList args = SmartCutEngine::buildKeyframeProbeCommand(ffprobe, m_inputFile);
    m_probeProcess->start(args.first(), args.mid(1));
}

void VideoClipTab::onKeyframeProbeFinished(int exitCode)
{
    if (exitCode != 0 || !m_probeProcess) return;
    const QString output = QString::fromUtf8(m_probeProcess->readAllStandardOutput());
    m_keyframes = SmartCutEngine::parseKeyframeOutput(output);
    if (m_trimPlayer) {
        m_trimPlayer->setKeyframes(m_keyframes);
    }
    const bool hasKeyframes = !m_keyframes.isEmpty();
    if (m_prevKeyframeBtn) m_prevKeyframeBtn->setEnabled(hasKeyframes);
    if (m_nextKeyframeBtn) m_nextKeyframeBtn->setEnabled(hasKeyframes);
    if (m_snapInButton) m_snapInButton->setEnabled(hasKeyframes);
    if (m_snapOutButton) m_snapOutButton->setEnabled(hasKeyframes);
    updateKeyframeStatus();
}

void VideoClipTab::updateKeyframeStatus()
{
    if (!m_keyframeStatusLabel) return;
    if (m_keyframes.isEmpty()) {
        m_keyframeStatusLabel->clear();
        return;
    }
    const double inVal = m_trimStart ? m_trimStart->value() : 0.0;
    const double outVal = m_trimEnd ? m_trimEnd->value() : 0.0;
    const bool inAligned = SmartCutEngine::isKeyframeAligned(inVal, m_keyframes);
    const bool outAligned = SmartCutEngine::isKeyframeAligned(outVal, m_keyframes);

    if (inAligned && outAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.bothAligned").arg(m_keyframes.size()));
    } else if (inAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.inAligned").arg(m_keyframes.size()));
    } else if (outAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.outAligned").arg(m_keyframes.size()));
    } else {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.notAligned").arg(m_keyframes.size()));
    }
}

void VideoClipTab::openLosslessCutStudio()
{
    if (m_cutDialog) {
        if (!m_inputFile.isEmpty()) {
            m_cutDialog->loadFile(m_inputFile);
        }
        m_cutDialog->raise();
        m_cutDialog->activateWindow();
        return;
    }
    m_cutDialog = new LosslessCutDialog(m_locator, m_probeService, m_queue, this);
    m_cutDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_cutDialog->loadFile(m_inputFile);
    }
    m_cutDialog->show();
}

void VideoClipTab::trim()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_trimFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_trimFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_trim.{format}"), ext);
    const auto mode = static_cast<CutMode>(m_trimMode->currentData().toInt());

    QStringList args;
    if (mode == CutMode::KeyframeLossless || m_trimCopy->isChecked()) {
        args = SmartCutEngine::buildLosslessCut(m_inputFile, output,
                                                m_trimStart->value(), m_trimEnd->value(),
                                                m_trimFormat->currentText());
    } else if (mode == CutMode::SmartCut) {
        SmartCutPlan plan = SmartCutEngine::planSmartCut(m_inputFile, output,
                                                         m_trimStart->value(), m_trimEnd->value(),
                                                         m_keyframes);
        if (plan.isValid && !plan.needsConcat) {
            args = SmartCutEngine::buildChunkCommand(m_inputFile, plan.chunks.first());
        } else {
            args = SmartCutEngine::buildLosslessCut(m_inputFile, output,
                                                    m_trimStart->value(), m_trimEnd->value(),
                                                    m_trimFormat->currentText());
        }
    } else {
        args = SmartCutEngine::buildAccurateReencode(m_inputFile, output,
                                                     m_trimStart->value(), m_trimEnd->value(),
                                                     QString(), 18, m_trimFormat->currentText());
    }

    enqueue(trId("video.job.trim").arg(QFileInfo(m_inputFile).fileName()), args);
}

void VideoClipTab::adjust()
{
    if (m_inputFile.isEmpty()) return;

    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_adj.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    enqueue(trId("video.job.adjust").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildVideoAdjust(
                m_inputFile, output,
                m_adjCropX->value(), m_adjCropY->value(),
                m_adjCropW->value(), m_adjCropH->value(),
                m_adjResizeW->value(), m_adjResizeH->value(),
                m_adjRotateCombo->currentText().toInt(),
                m_adjQualitySpin->value()));
}

void VideoClipTab::detectAutoCrop()
{
    if (m_inputFile.isEmpty()) {
        m_autoCropStatus->setText(trId("video.autoCrop.noInput"));
        return;
    }

    m_autoCropButton->setEnabled(false);
    m_autoCropStatus->setText(trId("video.autoCrop.detecting"));

    const QString inputPath = m_inputFile;
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const double duration = (m_trimPlayer && m_trimPlayer->durationSec() > 0)
                                ? m_trimPlayer->durationSec()
                                : 0.0;
    const double seekSec = (duration > 0.0)
                               ? qMin(1.0, duration * 0.1)
                                : 1.0;
    const QStringList args = m_builder->buildCropdetect(inputPath, seekSec, 25);

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        const QString output = watcher->result();
        watcher->deleteLater();
        m_autoCropButton->setEnabled(true);

        const FFmpegCommandBuilder::CropArea crop = FFmpegCommandBuilder::parseCropdetectOutput(output);
        if (crop.isValid()) {
            m_adjCropX->setValue(crop.x);
            m_adjCropY->setValue(crop.y);
            m_adjCropW->setValue(crop.w);
            m_adjCropH->setValue(crop.h);
            m_autoCropStatus->setText(QStringLiteral("%1: %2x%3 (+%4,+%5)")
                                          .arg(trId("video.autoCrop.found"))
                                          .arg(crop.w)
                                          .arg(crop.h)
                                          .arg(crop.x)
                                          .arg(crop.y));
        } else {
            m_autoCropStatus->setText(trId("video.autoCrop.none"));
        }
    });

    QFuture<QString> future = QtConcurrent::run([ffmpegPath, args]() -> QString {
        QProcess proc;
        proc.start(ffmpegPath, args);
        proc.waitForFinished(10000);
        return QString::fromUtf8(proc.readAllStandardError());
    });
    watcher->setFuture(future);
}

void VideoClipTab::joinVideos()
{
    if (m_joinFiles.size() < 2) return;
    const QString output = OutputNaming::build(m_joinFiles.first(),
                                               QStringLiteral("{name}_joined.{format}"),
                                               QFileInfo(m_joinFiles.first()).suffix());
    enqueue(trId("video.job.join").arg(m_joinFiles.size()),
            m_builder->buildVideoJoin(m_joinFiles, output));
}

void VideoClipTab::retranslateUi()
{
    m_trimTitleLabel->setText(trId("video.trim"));
    if (m_trimPlayer) m_trimPlayer->retranslateUi();
    m_setInButton->setText(trId("video.trim.setIn"));
    m_setOutButton->setText(trId("video.trim.setOut"));
    m_previewTrimButton->setText(trId("video.trim.preview"));
    m_trimStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_trimEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_trimModeCaption->setText(trId("cut.mode.caption") + QStringLiteral(":"));
    m_trimMode->setItemText(0, trId("cut.mode.lossless"));
    m_trimMode->setItemText(1, trId("cut.mode.smart"));
    m_trimMode->setItemText(2, trId("cut.mode.accurate"));
    m_snapInButton->setText(trId("cut.snap.in"));
    m_snapOutButton->setText(trId("cut.snap.out"));
    m_openStudioButton->setText(trId("cut.open.studio"));
    m_trimFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_trimCopyCaption->setText(trId("video.trim.copy") + QStringLiteral(":"));
    m_trimButton->setText(trId("convert.addToQueue"));
    updateKeyframeStatus();

    m_adjTitleLabel->setText(trId("video.adjust"));
    m_adjCropCaption->setText(trId("image.crop") + QStringLiteral(":"));
    m_autoCropButton->setText(trId("video.autoCrop"));
    m_adjResizeCaption->setText(trId("image.resize") + QStringLiteral(":"));
    m_adjRotateCaption->setText(trId("image.rotate") + QStringLiteral(":"));
    m_adjQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_adjButton->setText(trId("convert.addToQueue"));

    m_joinTitleLabel->setText(trId("video.join"));
    m_joinAddButton->setText(trId("common.open"));
    m_joinRemoveButton->setText(trId("common.delete"));
    m_joinButton->setText(trId("convert.addToQueue"));
}
