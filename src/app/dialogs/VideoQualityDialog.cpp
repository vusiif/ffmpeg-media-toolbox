#include "VideoQualityDialog.h"

#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "media/MediaFile.h"
#include "media/MediaProbeService.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QTextStream>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

VideoQualityDialog::VideoQualityDialog(const QString &referencePath,
                                       const QString &distortedPath,
                                       const FFmpegLocator *locator,
                                       MediaProbeService *probeService,
                                       QWidget *parent)
    : ElaDialog(parent)
    , m_referencePath(referencePath)
    , m_distortedPath(distortedPath)
    , m_locator(locator)
    , m_probeService(probeService)
    , m_evaluator(new VideoQualityEvaluator(locator, this))
{
    setupUi();

    connect(m_evaluator, &VideoQualityEvaluator::progressChanged,
            this, &VideoQualityDialog::onEvaluationProgress);
    connect(m_evaluator, &VideoQualityEvaluator::finished,
            this, &VideoQualityDialog::onEvaluationFinished);

    if (!m_referencePath.isEmpty() && m_probeService) {
        const MediaFile info = m_probeService->probe(m_referencePath);
        m_referenceDuration = info.duration;
        if (!info.videoStreams.isEmpty()) {
            m_refWidth = info.videoStreams.first().width;
            m_refHeight = info.videoStreams.first().height;
        }
    }

    retranslateUi();
}

VideoQualityDialog::~VideoQualityDialog() = default;

void VideoQualityDialog::setupUi()
{
    resize(760, 600);
    setWindowTitle(QString());

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 36, 24, 24);
    rootLayout->setSpacing(14);

    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(20);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    rootLayout->addWidget(m_titleLabel);

    // Reference file row
    auto *refRow = new QHBoxLayout();
    m_refCaption = new ElaText(QString(), 13, this);
    m_refEdit = new ElaLineEdit(this);
    m_refEdit->setReadOnly(true);
    m_refEdit->setText(m_referencePath);
    m_refBrowseBtn = new ElaPushButton(this);
    m_refBrowseBtn->setFixedHeight(32);
    connect(m_refBrowseBtn, &ElaPushButton::clicked, this, &VideoQualityDialog::onBrowseReference);
    refRow->addWidget(m_refCaption);
    refRow->addWidget(m_refEdit, 1);
    refRow->addWidget(m_refBrowseBtn);
    rootLayout->addLayout(refRow);

    // Distorted file row
    auto *distRow = new QHBoxLayout();
    m_distCaption = new ElaText(QString(), 13, this);
    m_distEdit = new ElaLineEdit(this);
    m_distEdit->setReadOnly(true);
    m_distEdit->setText(m_distortedPath);
    m_distBrowseBtn = new ElaPushButton(this);
    m_distBrowseBtn->setFixedHeight(32);
    connect(m_distBrowseBtn, &ElaPushButton::clicked, this, &VideoQualityDialog::onBrowseDistorted);
    distRow->addWidget(m_distCaption);
    distRow->addWidget(m_distEdit, 1);
    distRow->addWidget(m_distBrowseBtn);
    rootLayout->addLayout(distRow);

    // Options Row
    auto *optionsRow = new QHBoxLayout();
    optionsRow->setSpacing(12);

    m_modeCaption = new ElaText(QString(), 13, this);
    m_modeCombo = new ElaComboBox(this);
    m_modeCombo->addItem(QStringLiteral("Quick 10s Sample"));
    m_modeCombo->addItem(QStringLiteral("Full Video Stream"));

    m_vmafCheck = new ElaCheckBox(this);
    m_vmafCheck->setChecked(true);
    m_ssimCheck = new ElaCheckBox(this);
    m_ssimCheck->setChecked(true);
    m_psnrCheck = new ElaCheckBox(this);
    m_psnrCheck->setChecked(true);
    m_fastSubsampleCheck = new ElaCheckBox(this);
    m_fastSubsampleCheck->setChecked(true);

    optionsRow->addWidget(m_modeCaption);
    optionsRow->addWidget(m_modeCombo);
    optionsRow->addWidget(m_vmafCheck);
    optionsRow->addWidget(m_ssimCheck);
    optionsRow->addWidget(m_psnrCheck);
    optionsRow->addWidget(m_fastSubsampleCheck);
    optionsRow->addStretch();
    rootLayout->addLayout(optionsRow);

    // Results Card
    m_resultCard = new ElaScrollPageArea(this);
    m_resultCard->setMinimumHeight(200);
    auto *cardLayout = new QVBoxLayout(m_resultCard);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(12);

    auto *scoreRow = new QHBoxLayout();
    scoreRow->setSpacing(20);

    // VMAF large score
    auto *vmafCol = new QVBoxLayout();
    auto *vmafTitle = new ElaText(QStringLiteral("VMAF (0 - 100)"), 13, m_resultCard);
    vmafTitle->setStyleSheet(QStringLiteral("color: #888888;"));
    m_vmafValueLabel = new ElaText(QStringLiteral("--"), 26, m_resultCard);
    m_vmafValueLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_vmafRatingBadge = new ElaText(QStringLiteral("Pending"), 12, m_resultCard);
    m_vmafRatingBadge->setStyleSheet(QStringLiteral("color: #888888; font-weight: bold;"));
    vmafCol->addWidget(vmafTitle);
    vmafCol->addWidget(m_vmafValueLabel);
    vmafCol->addWidget(m_vmafRatingBadge);
    scoreRow->addLayout(vmafCol, 1);

    // SSIM score
    auto *ssimCol = new QVBoxLayout();
    auto *ssimTitle = new ElaText(QStringLiteral("SSIM (0 - 1.0)"), 13, m_resultCard);
    ssimTitle->setStyleSheet(QStringLiteral("color: #888888;"));
    m_ssimValueLabel = new ElaText(QStringLiteral("--"), 22, m_resultCard);
    m_ssimValueLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    ssimCol->addWidget(ssimTitle);
    ssimCol->addWidget(m_ssimValueLabel);
    ssimCol->addStretch();
    scoreRow->addLayout(ssimCol, 1);

    // PSNR score
    auto *psnrCol = new QVBoxLayout();
    auto *psnrTitle = new ElaText(QStringLiteral("PSNR (dB)"), 13, m_resultCard);
    psnrTitle->setStyleSheet(QStringLiteral("color: #888888;"));
    m_psnrValueLabel = new ElaText(QStringLiteral("--"), 22, m_resultCard);
    m_psnrValueLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    psnrCol->addWidget(psnrTitle);
    psnrCol->addWidget(m_psnrValueLabel);
    psnrCol->addStretch();
    scoreRow->addLayout(psnrCol, 1);

    cardLayout->addLayout(scoreRow);

    m_guideText = new ElaText(QString(), 12, m_resultCard);
    m_guideText->setStyleSheet(QStringLiteral("color: #777777;"));
    m_guideText->setWordWrap(true);
    cardLayout->addWidget(m_guideText);

    rootLayout->addWidget(m_resultCard, 1);

    // Progress bar & Status
    m_progressBar = new ElaProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    rootLayout->addWidget(m_progressBar);

    m_statusLabel = new ElaText(QString(), 12, this);
    m_statusLabel->setWordWrap(true);
    rootLayout->addWidget(m_statusLabel);

    // Action buttons row
    auto *actionRow = new QHBoxLayout();
    m_startBtn = new ElaPushButton(this);
    m_startBtn->setFixedHeight(36);
    m_startBtn->setMinimumWidth(130);
    connect(m_startBtn, &ElaPushButton::clicked, this, &VideoQualityDialog::onStartEvaluation);

    m_cancelBtn = new ElaPushButton(this);
    m_cancelBtn->setFixedHeight(36);
    m_cancelBtn->setEnabled(false);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &VideoQualityDialog::onCancelEvaluation);

    m_exportBtn = new ElaPushButton(this);
    m_exportBtn->setFixedHeight(36);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &ElaPushButton::clicked, this, &VideoQualityDialog::onExportReport);

    m_closeBtn = new ElaPushButton(this);
    m_closeBtn->setFixedHeight(36);
    connect(m_closeBtn, &ElaPushButton::clicked, this, &QDialog::accept);

    actionRow->addWidget(m_startBtn);
    actionRow->addWidget(m_cancelBtn);
    actionRow->addWidget(m_exportBtn);
    actionRow->addStretch();
    actionRow->addWidget(m_closeBtn);
    rootLayout->addLayout(actionRow);
}

void VideoQualityDialog::onBrowseReference()
{
    const QString p = QFileDialog::getOpenFileName(
        this, trId("quality.browseRef"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi);;All files (*)"));
    if (p.isEmpty()) return;
    m_referencePath = p;
    m_refEdit->setText(p);
    if (m_probeService) {
        const MediaFile info = m_probeService->probe(p);
        m_referenceDuration = info.duration;
        if (!info.videoStreams.isEmpty()) {
            m_refWidth = info.videoStreams.first().width;
            m_refHeight = info.videoStreams.first().height;
        }
    }
}

void VideoQualityDialog::onBrowseDistorted()
{
    const QString p = QFileDialog::getOpenFileName(
        this, trId("quality.browseDist"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi);;All files (*)"));
    if (p.isEmpty()) return;
    m_distortedPath = p;
    m_distEdit->setText(p);
}

void VideoQualityDialog::onStartEvaluation()
{
    if (m_referencePath.isEmpty() || m_distortedPath.isEmpty()) return;

    m_startBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_exportBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(trId("quality.evaluating"));

    m_vmafValueLabel->setText(QStringLiteral("--"));
    m_vmafRatingBadge->setText(QStringLiteral("Computing..."));
    m_ssimValueLabel->setText(QStringLiteral("--"));
    m_psnrValueLabel->setText(QStringLiteral("--"));

    VideoQualityOptions opts;
    opts.calculateVmaf = m_vmafCheck->isChecked();
    opts.calculateSsim = m_ssimCheck->isChecked();
    opts.calculatePsnr = m_psnrCheck->isChecked();
    opts.subsample = m_fastSubsampleCheck->isChecked() ? 2 : 1;

    if (m_modeCombo->currentIndex() == 0) {
        // Quick 10s Sample
        opts.sampleStart = m_referenceDuration > 20.0 ? m_referenceDuration * 0.25 : 0.0;
        opts.sampleDuration = 10.0;
    } else {
        opts.sampleDuration = 0.0; // Full
    }

    m_evaluator->startEvaluation(m_referencePath, m_distortedPath, opts, m_referenceDuration, m_refWidth, m_refHeight);
}

void VideoQualityDialog::onCancelEvaluation()
{
    m_evaluator->cancel();
    m_startBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);
    m_statusLabel->setText(trId("quality.cancelled"));
}

void VideoQualityDialog::onEvaluationProgress(double fraction)
{
    m_progressBar->setValue(static_cast<int>(fraction * 100.0));
}

void VideoQualityDialog::onEvaluationFinished(const VideoQualityResult &result)
{
    m_lastResult = result;
    m_startBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);

    if (result.success) {
        updateScoreDisplay(result);
        m_statusLabel->setText(trId("quality.finished"));
        m_exportBtn->setEnabled(true);
    } else {
        m_statusLabel->setText(trId("quality.failed") + QStringLiteral(": ") + result.errorMessage);
        m_vmafRatingBadge->setText(QStringLiteral("Error"));
    }
}

void VideoQualityDialog::updateScoreDisplay(const VideoQualityResult &res)
{
    if (res.vmaf >= 0.0) {
        m_vmafValueLabel->setText(QString::number(res.vmaf, 'f', 2));
        m_vmafRatingBadge->setText(res.vmafRating());
        const QColor c = res.ratingColor();
        m_vmafValueLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(c.name()));
        m_vmafRatingBadge->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(c.name()));
    } else {
        m_vmafValueLabel->setText(QStringLiteral("N/A"));
        m_vmafRatingBadge->setText(QString());
    }

    if (res.ssim >= 0.0) {
        m_ssimValueLabel->setText(QString::number(res.ssim, 'f', 4));
    } else {
        m_ssimValueLabel->setText(QStringLiteral("N/A"));
    }

    if (res.psnr >= 0.0) {
        m_psnrValueLabel->setText(QString::number(res.psnr, 'f', 2) + QStringLiteral(" dB"));
    } else {
        m_psnrValueLabel->setText(QStringLiteral("N/A"));
    }
}

void VideoQualityDialog::onExportReport()
{
    if (m_referencePath.isEmpty() || !m_lastResult.success) return;

    const QString defName = QFileInfo(m_distortedPath).baseName() + QStringLiteral("_vmaf_report.txt");
    const QString savePath = QFileDialog::getSaveFileName(
        this, trId("quality.export"), defName,
        QStringLiteral("Text Report (*.txt);;All Files (*)"));
    if (savePath.isEmpty()) return;

    VideoQualityOptions opts;
    opts.calculateVmaf = m_vmafCheck->isChecked();
    opts.calculateSsim = m_ssimCheck->isChecked();
    opts.calculatePsnr = m_psnrCheck->isChecked();
    if (m_modeCombo->currentIndex() == 0) {
        opts.sampleDuration = 10.0;
        opts.sampleStart = m_referenceDuration > 20.0 ? m_referenceDuration * 0.25 : 0.0;
    }

    const QString report = VideoQualityEvaluator::generateReport(m_referencePath, m_distortedPath, m_lastResult, opts);
    QFile f(savePath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << report;
        f.close();
    }
}

void VideoQualityDialog::retranslateUi()
{
    m_titleLabel->setText(trId("quality.dialog.title"));
    m_refCaption->setText(trId("quality.refFile") + QStringLiteral(":"));
    m_refBrowseBtn->setText(trId("common.browse"));
    m_distCaption->setText(trId("quality.distFile") + QStringLiteral(":"));
    m_distBrowseBtn->setText(trId("common.browse"));

    m_modeCaption->setText(trId("quality.mode") + QStringLiteral(":"));
    m_modeCombo->setItemText(0, trId("quality.mode.sample10s"));
    m_modeCombo->setItemText(1, trId("quality.mode.full"));

    m_vmafCheck->setText(QStringLiteral("VMAF"));
    m_ssimCheck->setText(QStringLiteral("SSIM"));
    m_psnrCheck->setText(QStringLiteral("PSNR"));
    m_fastSubsampleCheck->setText(trId("quality.fastSubsample"));

    m_guideText->setText(trId("quality.ratingGuide"));

    m_startBtn->setText(trId("quality.start"));
    m_cancelBtn->setText(trId("common.cancel"));
    m_exportBtn->setText(trId("quality.export"));
    m_closeBtn->setText(trId("common.close"));
}
