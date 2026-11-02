#include "ABCompareDialog.h"

#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QScrollArea>
#include <QUuid>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "ElaDoubleSpinBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"

ABCompareDialog::ABCompareDialog(const QString &sourcePath,
                                 const QString &previewPath,
                                 double duration,
                                 const FFmpegLocator *locator,
                                 QWidget *parent)
    : ElaDialog(parent)
    , m_sourcePath(sourcePath)
    , m_previewPath(previewPath)
    , m_duration(qMax(0.0, duration))
    , m_locator(locator)
    , m_evaluator(new VideoQualityEvaluator(locator, this))
{
    const QString token = QUuid::createUuid().toString(QUuid::Id128).left(8);
    m_sourceTempImg = QDir::temp().filePath(QStringLiteral("ab_src_%1.png").arg(token));
    m_previewTempImg = QDir::temp().filePath(QStringLiteral("ab_prev_%1.png").arg(token));

    m_watcher = new QFutureWatcher<QPair<bool, bool>>(this);
    connect(m_watcher, &QFutureWatcher<QPair<bool, bool>>::finished,
            this, &ABCompareDialog::onCaptureFinished);
    connect(m_evaluator, &VideoQualityEvaluator::finished,
            this, &ABCompareDialog::onQualityEvaluationFinished);

    setupUi();
    retranslateUi();
    captureFrames();
}

ABCompareDialog::~ABCompareDialog()
{
    if (m_watcher->isRunning()) {
        m_watcher->waitForFinished();
    }
    QFile::remove(m_sourceTempImg);
    QFile::remove(m_previewTempImg);
}

void ABCompareDialog::setupUi()
{
    resize(1000, 680);
    setWindowTitle(QString());

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 36, 20, 20);
    rootLayout->setSpacing(12);

    // Header title
    m_titleText = new ElaText(QString(), 16, this);
    m_titleText->setStyleSheet(QStringLiteral("font-weight: bold;"));
    rootLayout->addWidget(m_titleText);

    // Controls Row: Timestamp & Quick Jump Buttons
    auto *ctrlRow = new QHBoxLayout();
    ctrlRow->setSpacing(8);

    m_timeCaption = new ElaText(QString(), 13, this);
    ctrlRow->addWidget(m_timeCaption);

    m_timeSpin = new ElaDoubleSpinBox(this);
    m_timeSpin->setRange(0.0, m_duration > 0 ? m_duration : 36000.0);
    m_timeSpin->setSingleStep(1.0);
    m_timeSpin->setDecimals(1);
    m_timeSpin->setSuffix(QStringLiteral(" s"));
    if (m_duration > 0) {
        m_timeSpin->setValue(m_duration * 0.25);
    } else {
        m_timeSpin->setValue(5.0);
    }
    ctrlRow->addWidget(m_timeSpin);

    m_pct10Btn = new ElaPushButton(QStringLiteral("10%"), this);
    m_pct25Btn = new ElaPushButton(QStringLiteral("25%"), this);
    m_pct50Btn = new ElaPushButton(QStringLiteral("50%"), this);
    m_pct75Btn = new ElaPushButton(QStringLiteral("75%"), this);
    m_pct10Btn->setMaximumWidth(55);
    m_pct25Btn->setMaximumWidth(55);
    m_pct50Btn->setMaximumWidth(55);
    m_pct75Btn->setMaximumWidth(55);

    connect(m_pct10Btn, &ElaPushButton::clicked, this, [this]() {
        if (m_duration > 0) m_timeSpin->setValue(m_duration * 0.10);
    });
    connect(m_pct25Btn, &ElaPushButton::clicked, this, [this]() {
        if (m_duration > 0) m_timeSpin->setValue(m_duration * 0.25);
    });
    connect(m_pct50Btn, &ElaPushButton::clicked, this, [this]() {
        if (m_duration > 0) m_timeSpin->setValue(m_duration * 0.50);
    });
    connect(m_pct75Btn, &ElaPushButton::clicked, this, [this]() {
        if (m_duration > 0) m_timeSpin->setValue(m_duration * 0.75);
    });

    ctrlRow->addWidget(m_pct10Btn);
    ctrlRow->addWidget(m_pct25Btn);
    ctrlRow->addWidget(m_pct50Btn);
    ctrlRow->addWidget(m_pct75Btn);

    m_refreshBtn = new ElaPushButton(this);
    connect(m_refreshBtn, &ElaPushButton::clicked, this, &ABCompareDialog::captureFrames);
    ctrlRow->addWidget(m_refreshBtn);

    m_evalQualityBtn = new ElaPushButton(this);
    connect(m_evalQualityBtn, &ElaPushButton::clicked, this, &ABCompareDialog::onEvaluateQuality);
    ctrlRow->addWidget(m_evalQualityBtn);

    ctrlRow->addStretch();
    rootLayout->addLayout(ctrlRow);

    // Comparison Viewport (Left / Right)
    auto *viewLayout = new QHBoxLayout();
    viewLayout->setSpacing(14);

    // Left: Source
    auto *leftCol = new QVBoxLayout();
    m_leftHeader = new ElaText(QString(), 14, this);
    m_leftHeader->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    leftCol->addWidget(m_leftHeader);

    m_leftScroll = new QScrollArea(this);
    m_leftScroll->setWidgetResizable(true);
    m_leftScroll->setStyleSheet(QStringLiteral("background-color: #1a1a1a; border-radius: 8px;"));
    m_leftImageLabel = new QLabel(m_leftScroll);
    m_leftImageLabel->setAlignment(Qt::AlignCenter);
    m_leftImageLabel->setScaledContents(false);
    m_leftScroll->setWidget(m_leftImageLabel);
    leftCol->addWidget(m_leftScroll, 1);
    viewLayout->addLayout(leftCol, 1);

    // Right: Preview / Encoded
    auto *rightCol = new QVBoxLayout();
    m_rightHeader = new ElaText(QString(), 14, this);
    m_rightHeader->setStyleSheet(QStringLiteral("font-weight: bold; color: #107c10;"));
    rightCol->addWidget(m_rightHeader);

    m_rightScroll = new QScrollArea(this);
    m_rightScroll->setWidgetResizable(true);
    m_rightScroll->setStyleSheet(QStringLiteral("background-color: #1a1a1a; border-radius: 8px;"));
    m_rightImageLabel = new QLabel(m_rightScroll);
    m_rightImageLabel->setAlignment(Qt::AlignCenter);
    m_rightImageLabel->setScaledContents(false);
    m_rightScroll->setWidget(m_rightImageLabel);
    rightCol->addWidget(m_rightScroll, 1);
    viewLayout->addLayout(rightCol, 1);

    rootLayout->addLayout(viewLayout, 1);

    // Bottom Status & Action Row
    auto *bottomRow = new QHBoxLayout();
    m_statusLabel = new ElaText(QString(), 12, this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    bottomRow->addWidget(m_statusLabel, 1);

    m_qualityScoreLabel = new ElaText(QString(), 13, this);
    bottomRow->addWidget(m_qualityScoreLabel);

    m_closeBtn = new ElaPushButton(this);
    connect(m_closeBtn, &ElaPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(m_closeBtn);
    rootLayout->addLayout(bottomRow);
}

void ABCompareDialog::retranslateUi()
{
    m_titleText->setText(trId("abcompare.title"));
    m_timeCaption->setText(trId("abcompare.timestamp") + QStringLiteral(":"));
    m_refreshBtn->setText(trId("abcompare.refresh"));
    m_evalQualityBtn->setText(trId("abcompare.evalQuality"));
    m_leftHeader->setText(trId("abcompare.original") + QStringLiteral(" (") + QFileInfo(m_sourcePath).fileName() + QStringLiteral(")"));
    m_rightHeader->setText(trId("abcompare.preview") + QStringLiteral(" (") + (m_previewPath.isEmpty() ? trId("abcompare.notAvailable") : QFileInfo(m_previewPath).fileName()) + QStringLiteral(")"));
    m_closeBtn->setText(trId("common.close"));
}

bool ABCompareDialog::extractFrame(const QString &ffmpegPath, const QString &input,
                                   double timeSec, const QString &outputImage)
{
    if (ffmpegPath.isEmpty() || input.isEmpty() || outputImage.isEmpty()) {
        return false;
    }
    QFileInfo fi(input);
    if (!fi.exists() || fi.size() == 0) {
        return false;
    }

    QProcess proc;
    QStringList args;
    args << QStringLiteral("-hide_banner")
         << QStringLiteral("-ss") << QString::number(timeSec, 'f', 2)
         << QStringLiteral("-i") << input
         << QStringLiteral("-frames:v") << QStringLiteral("1")
         << QStringLiteral("-y")
         << outputImage;

    proc.start(ffmpegPath, args);
    if (!proc.waitForFinished(6000)) {
        proc.kill();
        return false;
    }
    return proc.exitCode() == 0 && QFile::exists(outputImage);
}

void ABCompareDialog::captureFrames()
{
    if (m_watcher->isRunning()) return;

    const double timeSec = m_timeSpin->value();
    const QString ffPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QString src = m_sourcePath;
    const QString prev = m_previewPath;
    const QString srcOut = m_sourceTempImg;
    const QString prevOut = m_previewTempImg;

    m_refreshBtn->setEnabled(false);
    m_statusLabel->setText(trId("abcompare.capturing"));

    m_watcher->setFuture(QtConcurrent::run([ffPath, src, prev, timeSec, srcOut, prevOut]() {
        const bool srcOk = extractFrame(ffPath, src, timeSec, srcOut);
        const bool prevOk = !prev.isEmpty() && extractFrame(ffPath, prev, timeSec, prevOut);
        return qMakePair(srcOk, prevOk);
    }));
}

void ABCompareDialog::onCaptureFinished()
{
    m_refreshBtn->setEnabled(true);
    const QPair<bool, bool> res = m_watcher->result();

    if (res.first && QFile::exists(m_sourceTempImg)) {
        QPixmap pm(m_sourceTempImg);
        if (!pm.isNull()) {
            m_leftImageLabel->setPixmap(pm.scaled(m_leftScroll->size() - QSize(10, 10),
                                                  Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        m_leftImageLabel->setText(trId("abcompare.failed"));
    }

    if (res.second && QFile::exists(m_previewTempImg)) {
        QPixmap pm(m_previewTempImg);
        if (!pm.isNull()) {
            m_rightImageLabel->setPixmap(pm.scaled(m_rightScroll->size() - QSize(10, 10),
                                                   Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        m_rightImageLabel->setText(m_previewPath.isEmpty() ? trId("abcompare.noSampleHint") : trId("abcompare.failed"));
    }

    m_statusLabel->setText(trId("abcompare.ready") + QStringLiteral(" @ %1s").arg(m_timeSpin->value(), 0, 'f', 1));
}

void ABCompareDialog::onEvaluateQuality()
{
    if (m_sourcePath.isEmpty() || m_previewPath.isEmpty()) return;

    m_evalQualityBtn->setEnabled(false);
    m_qualityScoreLabel->setText(trId("quality.evaluating"));

    VideoQualityOptions opts;
    opts.calculateVmaf = true;
    opts.calculateSsim = true;
    opts.calculatePsnr = true;
    opts.sampleStart = qMax(0.0, m_timeSpin->value() - 2.5);
    opts.sampleDuration = 5.0;

    m_evaluator->startEvaluation(m_sourcePath, m_previewPath, opts, 5.0);
}

void ABCompareDialog::onQualityEvaluationFinished(const VideoQualityResult &res)
{
    m_evalQualityBtn->setEnabled(true);
    if (res.success) {
        m_qualityScoreLabel->setText(res.summaryText());
        m_qualityScoreLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(res.ratingColor().name()));
    } else {
        m_qualityScoreLabel->setText(trId("quality.failed"));
    }
}
