#include "GifStudioDialog.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"

#include "app/widgets/VideoPlayerWidget.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

GifStudioDialog::GifStudioDialog(JobQueue *queue, FFmpegLocator *locator, QWidget *parent)
    : QDialog(parent)
    , m_queue(queue)
    , m_locator(locator)
{
    resize(1100, 750);
    setWindowTitle(trId("animation.dialog.title"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // 1. Header
    auto *headerLayout = new QVBoxLayout();
    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(22);
    m_subtitleLabel = new ElaText(this);
    m_subtitleLabel->setTextPixelSize(13);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    root->addLayout(headerLayout);

    // 2. Main Two-Column Body
    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(16);

    // Left Column: Player & Range Selection & Live Stats
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(10);

    // Media File Card
    auto *inputCard = new ElaScrollPageArea(this);
    auto *inputLayout = new QHBoxLayout(inputCard);
    inputLayout->setContentsMargins(10, 8, 10, 8);
    m_pathEdit = new ElaLineEdit(inputCard);
    m_pathEdit->setReadOnly(true);
    m_browseBtn = new ElaPushButton(inputCard);
    connect(m_browseBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onBrowseFile);
    inputLayout->addWidget(m_pathEdit, 1);
    inputLayout->addWidget(m_browseBtn);
    leftCol->addWidget(inputCard);

    // Video Player
    m_player = new VideoPlayerWidget(this);
    m_player->setMinimumHeight(280);
    leftCol->addWidget(m_player, 1);

    // Range Selection Card
    auto *rangeCard = new ElaScrollPageArea(this);
    auto *rangeLayout = new QVBoxLayout(rangeCard);
    rangeLayout->setContentsMargins(10, 10, 10, 10);
    rangeLayout->setSpacing(8);

    auto *btnRow = new QHBoxLayout();
    m_setInBtn = new ElaPushButton(trId("animation.setIn"), rangeCard);
    m_setOutBtn = new ElaPushButton(trId("animation.setOut"), rangeCard);
    m_previewRangeBtn = new ElaPushButton(trId("animation.previewRange"), rangeCard);
    connect(m_setInBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onSetInPoint);
    connect(m_setOutBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onSetOutPoint);
    connect(m_previewRangeBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onPreviewRange);
    btnRow->addWidget(m_setInBtn);
    btnRow->addWidget(m_setOutBtn);
    btnRow->addWidget(m_previewRangeBtn);
    rangeLayout->addLayout(btnRow);

    auto *spinRow = new QHBoxLayout();
    auto *startLayout = new QVBoxLayout();
    startLayout->addWidget(new ElaText(trId("animation.startTime"), rangeCard));
    m_startSpin = new ElaDoubleSpinBox(rangeCard);
    m_startSpin->setRange(0, 36000);
    m_startSpin->setValue(0);
    m_startSpin->setSuffix(QStringLiteral(" s"));
    connect(m_startSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &GifStudioDialog::onStartSpinChanged);
    startLayout->addWidget(m_startSpin);

    auto *durLayout = new QVBoxLayout();
    durLayout->addWidget(new ElaText(trId("animation.duration"), rangeCard));
    m_durationSpin = new ElaDoubleSpinBox(rangeCard);
    m_durationSpin->setRange(0.1, 3600);
    m_durationSpin->setValue(3.0);
    m_durationSpin->setSuffix(QStringLiteral(" s"));
    connect(m_durationSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &GifStudioDialog::onDurationSpinChanged);
    durLayout->addWidget(m_durationSpin);

    auto *endLayout = new QVBoxLayout();
    endLayout->addWidget(new ElaText(trId("animation.endTime"), rangeCard));
    m_endSpin = new ElaDoubleSpinBox(rangeCard);
    m_endSpin->setRange(0.1, 36000);
    m_endSpin->setValue(3.0);
    m_endSpin->setSuffix(QStringLiteral(" s"));
    connect(m_endSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &GifStudioDialog::onEndSpinChanged);
    endLayout->addWidget(m_endSpin);

    spinRow->addLayout(startLayout);
    spinRow->addLayout(durLayout);
    spinRow->addLayout(endLayout);
    rangeLayout->addLayout(spinRow);

    leftCol->addWidget(rangeCard);

    // Live Stats Banner
    m_estimateBanner = new QLabel(this);
    m_estimateBanner->setStyleSheet(
        QStringLiteral("background-color: #1e222d; border: 1px solid #0078d4; border-radius: 6px; "
                       "padding: 8px 14px; font-weight: bold; color: #e0e0e0;"));
    m_estimateBanner->setAlignment(Qt::AlignCenter);
    leftCol->addWidget(m_estimateBanner);

    bodyLayout->addLayout(leftCol, 5);

    // Right Column: Controls in Scroll Area
    auto *rightScroll = new ElaScrollArea(this);
    rightScroll->setWidgetResizable(true);
    auto *rightWidget = new QWidget(rightScroll);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(12);

    // Card 1: Format & Presets
    auto *fmtCard = new ElaScrollPageArea(rightWidget);
    auto *fmtForm = new QFormLayout(fmtCard);
    fmtForm->setSpacing(8);

    m_formatCombo = new ElaComboBox(fmtCard);
    m_formatCombo->addItem(QStringLiteral("GIF (.gif)"), static_cast<int>(Animation::OutputFormat::GIF));
    m_formatCombo->addItem(QStringLiteral("Animated WebP (.webp)"), static_cast<int>(Animation::OutputFormat::WebP));
    m_formatCombo->addItem(QStringLiteral("Animated PNG (.png)"), static_cast<int>(Animation::OutputFormat::APNG));
    connect(m_formatCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onFormatChanged);
    fmtForm->addRow(new ElaText(trId("animation.outputFormat") + QStringLiteral(":"), fmtCard), m_formatCombo);

    m_presetCombo = new ElaComboBox(fmtCard);
    m_presetCombo->addItem(trId("animation.preset.hq256"));
    m_presetCombo->addItem(trId("animation.preset.balanced128"));
    m_presetCombo->addItem(trId("animation.preset.sticker64"));
    m_presetCombo->addItem(trId("animation.preset.smooth60"));
    connect(m_presetCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onPresetChanged);
    fmtForm->addRow(new ElaText(trId("animation.preset") + QStringLiteral(":"), fmtCard), m_presetCombo);

    rightLayout->addWidget(fmtCard);

    // Card 2: Geometry & Playback
    auto *geomCard = new ElaScrollPageArea(rightWidget);
    auto *geomForm = new QFormLayout(geomCard);
    geomForm->setSpacing(8);

    m_resPresetCombo = new ElaComboBox(geomCard);
    m_resPresetCombo->addItem(QStringLiteral("480px (标清推荐)"), 480);
    m_resPresetCombo->addItem(QStringLiteral("360px (中等微动图)"), 360);
    m_resPresetCombo->addItem(QStringLiteral("240px (轻巧表情包)"), 240);
    m_resPresetCombo->addItem(QStringLiteral("720px (高清大图)"), 720);
    m_resPresetCombo->addItem(QStringLiteral("原始比例"), -1);
    connect(m_resPresetCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onResolutionPresetChanged);
    geomForm->addRow(new ElaText(trId("animation.resolution") + QStringLiteral(":"), geomCard), m_resPresetCombo);

    m_widthSpin = new ElaSpinBox(geomCard);
    m_widthSpin->setRange(64, 3840);
    m_widthSpin->setValue(480);
    connect(m_widthSpin, QOverload<int>::of(&ElaSpinBox::valueChanged),
            this, &GifStudioDialog::onWidthChanged);
    geomForm->addRow(new ElaText(trId("animation.customWidth") + QStringLiteral(":"), geomCard), m_widthSpin);

    m_fpsCombo = new ElaComboBox(geomCard);
    m_fpsCombo->addItem(QStringLiteral("10 fps (极速小体积)"), 10);
    m_fpsCombo->addItem(QStringLiteral("12 fps (经典定格)"), 12);
    m_fpsCombo->addItem(QStringLiteral("15 fps (标准推荐)"), 15);
    m_fpsCombo->addItem(QStringLiteral("20 fps (自然流畅)"), 20);
    m_fpsCombo->addItem(QStringLiteral("24 fps (电影感)"), 24);
    m_fpsCombo->addItem(QStringLiteral("30 fps (极致丝滑)"), 30);
    m_fpsCombo->setCurrentIndex(2); // 15 fps
    connect(m_fpsCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onFpsChanged);
    geomForm->addRow(new ElaText(trId("animation.fps") + QStringLiteral(":"), geomCard), m_fpsCombo);

    m_speedCombo = new ElaComboBox(geomCard);
    m_speedCombo->addItem(QStringLiteral("0.5x 慢放"), 0.5);
    m_speedCombo->addItem(QStringLiteral("0.75x 稍慢"), 0.75);
    m_speedCombo->addItem(QStringLiteral("1.0x 正常速度"), 1.0);
    m_speedCombo->addItem(QStringLiteral("1.25x 轻微加速"), 1.25);
    m_speedCombo->addItem(QStringLiteral("1.5x 加速"), 1.5);
    m_speedCombo->addItem(QStringLiteral("2.0x 双倍速"), 2.0);
    m_speedCombo->setCurrentIndex(2); // 1.0x
    connect(m_speedCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onSpeedChanged);
    geomForm->addRow(new ElaText(trId("animation.speed") + QStringLiteral(":"), geomCard), m_speedCombo);

    m_directionCombo = new ElaComboBox(geomCard);
    m_directionCombo->addItem(trId("animation.dir.normal"), static_cast<int>(Animation::PlaybackDirection::Normal));
    m_directionCombo->addItem(trId("animation.dir.reverse"), static_cast<int>(Animation::PlaybackDirection::Reverse));
    m_directionCombo->addItem(trId("animation.dir.pingpong"), static_cast<int>(Animation::PlaybackDirection::PingPong));
    connect(m_directionCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onDirectionChanged);
    geomForm->addRow(new ElaText(trId("animation.direction") + QStringLiteral(":"), geomCard), m_directionCombo);

    rightLayout->addWidget(geomCard);

    // Card 3: GIF Palette & Dither
    m_gifPaletteWidget = new ElaScrollPageArea(rightWidget);
    auto *palForm = new QFormLayout(m_gifPaletteWidget);
    palForm->setSpacing(8);

    m_colorsSpin = new ElaSpinBox(m_gifPaletteWidget);
    m_colorsSpin->setRange(32, 256);
    m_colorsSpin->setValue(256);
    connect(m_colorsSpin, QOverload<int>::of(&ElaSpinBox::valueChanged),
            this, &GifStudioDialog::onMaxColorsChanged);
    palForm->addRow(new ElaText(trId("animation.maxColors") + QStringLiteral(":"), m_gifPaletteWidget), m_colorsSpin);

    m_ditherCombo = new ElaComboBox(m_gifPaletteWidget);
    m_ditherCombo->addItem(trId("animation.dither.bayer"), static_cast<int>(Animation::DitherMode::Bayer));
    m_ditherCombo->addItem(trId("animation.dither.floyd"), static_cast<int>(Animation::DitherMode::FloydSteinberg));
    m_ditherCombo->addItem(trId("animation.dither.sierra"), static_cast<int>(Animation::DitherMode::Sierra2_4a));
    m_ditherCombo->addItem(trId("animation.dither.none"), static_cast<int>(Animation::DitherMode::None));
    connect(m_ditherCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onDitherModeChanged);
    palForm->addRow(new ElaText(trId("animation.dither") + QStringLiteral(":"), m_gifPaletteWidget), m_ditherCombo);

    auto *bayerRow = new QHBoxLayout();
    m_bayerSlider = new ElaSlider(Qt::Horizontal, m_gifPaletteWidget);
    m_bayerSlider->setRange(1, 5);
    m_bayerSlider->setValue(3);
    m_bayerLabel = new QLabel(QStringLiteral("3"), m_gifPaletteWidget);
    m_bayerLabel->setFixedWidth(20);
    connect(m_bayerSlider, &ElaSlider::valueChanged, this, &GifStudioDialog::onBayerScaleChanged);
    bayerRow->addWidget(m_bayerSlider, 1);
    bayerRow->addWidget(m_bayerLabel);
    palForm->addRow(new ElaText(trId("animation.bayerScale") + QStringLiteral(":"), m_gifPaletteWidget), bayerRow);

    m_statsModeCombo = new ElaComboBox(m_gifPaletteWidget);
    m_statsModeCombo->addItem(trId("animation.stats.diff"), static_cast<int>(Animation::PaletteStatsMode::Diff));
    m_statsModeCombo->addItem(trId("animation.stats.full"), static_cast<int>(Animation::PaletteStatsMode::Full));
    connect(m_statsModeCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &GifStudioDialog::onStatsModeChanged);
    palForm->addRow(new ElaText(trId("animation.statsMode") + QStringLiteral(":"), m_gifPaletteWidget), m_statsModeCombo);

    m_loopSpin = new ElaSpinBox(m_gifPaletteWidget);
    m_loopSpin->setRange(-1, 100);
    m_loopSpin->setValue(0);
    m_loopSpin->setSpecialValueText(trId("animation.loop.infinite"));
    connect(m_loopSpin, QOverload<int>::of(&ElaSpinBox::valueChanged),
            this, &GifStudioDialog::onLoopCountChanged);
    palForm->addRow(new ElaText(trId("animation.loopCount") + QStringLiteral(":"), m_gifPaletteWidget), m_loopSpin);

    rightLayout->addWidget(m_gifPaletteWidget);

    // Card 4: WebP Options
    m_webpWidget = new ElaScrollPageArea(rightWidget);
    auto *webpForm = new QFormLayout(m_webpWidget);
    webpForm->setSpacing(8);

    auto *qualityRow = new QHBoxLayout();
    m_webpQualitySlider = new ElaSlider(Qt::Horizontal, m_webpWidget);
    m_webpQualitySlider->setRange(10, 100);
    m_webpQualitySlider->setValue(75);
    m_webpQualityLabel = new QLabel(QStringLiteral("75"), m_webpWidget);
    m_webpQualityLabel->setFixedWidth(30);
    connect(m_webpQualitySlider, &ElaSlider::valueChanged, this, &GifStudioDialog::onWebpQualityChanged);
    qualityRow->addWidget(m_webpQualitySlider, 1);
    qualityRow->addWidget(m_webpQualityLabel);
    webpForm->addRow(new ElaText(trId("animation.webpQuality") + QStringLiteral(":"), m_webpWidget), qualityRow);

    m_webpLosslessCheck = new ElaCheckBox(trId("animation.webpLossless"), m_webpWidget);
    connect(m_webpLosslessCheck, &ElaCheckBox::toggled, this, &GifStudioDialog::onWebpLosslessToggled);
    webpForm->addRow(QString(), m_webpLosslessCheck);

    m_webpWidget->hide();
    rightLayout->addWidget(m_webpWidget);

    // Action Buttons
    auto *btnLayout = new QHBoxLayout();
    m_copyCmdBtn = new ElaPushButton(trId("animation.copyCmd"), rightWidget);
    connect(m_copyCmdBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onCopyCommand);

    m_addToQueueBtn = new ElaPushButton(trId("animation.addToQueue"), rightWidget);
    m_addToQueueBtn->setStyleSheet(QStringLiteral("background-color: #0078d4; color: white; font-weight: bold;"));
    connect(m_addToQueueBtn, &ElaPushButton::clicked, this, &GifStudioDialog::onAddToQueue);

    btnLayout->addWidget(m_copyCmdBtn);
    btnLayout->addWidget(m_addToQueueBtn);
    rightLayout->addLayout(btnLayout);

    rightScroll->setWidget(rightWidget);
    bodyLayout->addWidget(rightScroll, 4);

    root->addLayout(bodyLayout);

    retranslateUi();
    updateEstimateBanner();
}

GifStudioDialog::~GifStudioDialog() = default;

void GifStudioDialog::setVideoFile(const QString &filePath)
{
    m_videoPath = filePath;
    m_pathEdit->setText(filePath);
    if (!filePath.isEmpty() && m_player) {
        m_player->loadMedia(filePath);
    }
}

void GifStudioDialog::retranslateUi()
{
    setWindowTitle(trId("animation.dialog.title"));
    m_titleLabel->setText(trId("animation.dialog.title"));
    m_subtitleLabel->setText(trId("animation.dialog.subtitle"));
    m_browseBtn->setText(trId("common.browse"));

    m_setInBtn->setText(trId("animation.setIn"));
    m_setOutBtn->setText(trId("animation.setOut"));
    m_previewRangeBtn->setText(trId("animation.previewRange"));

    m_presetCombo->setItemText(0, trId("animation.preset.hq256"));
    m_presetCombo->setItemText(1, trId("animation.preset.balanced128"));
    m_presetCombo->setItemText(2, trId("animation.preset.sticker64"));
    m_presetCombo->setItemText(3, trId("animation.preset.smooth60"));

    m_directionCombo->setItemText(0, trId("animation.dir.normal"));
    m_directionCombo->setItemText(1, trId("animation.dir.reverse"));
    m_directionCombo->setItemText(2, trId("animation.dir.pingpong"));

    m_ditherCombo->setItemText(0, trId("animation.dither.bayer"));
    m_ditherCombo->setItemText(1, trId("animation.dither.floyd"));
    m_ditherCombo->setItemText(2, trId("animation.dither.sierra"));
    m_ditherCombo->setItemText(3, trId("animation.dither.none"));

    m_statsModeCombo->setItemText(0, trId("animation.stats.diff"));
    m_statsModeCombo->setItemText(1, trId("animation.stats.full"));

    m_copyCmdBtn->setText(trId("animation.copyCmd"));
    m_addToQueueBtn->setText(trId("animation.addToQueue"));
}

void GifStudioDialog::onBrowseFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("animation.selectVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.ts);;All files (*)"));
    if (!path.isEmpty()) {
        setVideoFile(path);
    }
}

void GifStudioDialog::onSetInPoint()
{
    if (m_player) {
        const double pos = m_player->currentPositionSec();
        m_startSpin->setValue(pos);
    }
}

void GifStudioDialog::onSetOutPoint()
{
    if (m_player) {
        const double pos = m_player->currentPositionSec();
        m_endSpin->setValue(pos);
    }
}

void GifStudioDialog::onPreviewRange()
{
    if (m_player) {
        m_player->seekToSec(m_startSpin->value());
        m_player->play();
    }
}

void GifStudioDialog::onStartSpinChanged(double val)
{
    m_config.startSec = val;
    m_endSpin->blockSignals(true);
    m_endSpin->setValue(m_config.startSec + m_config.durationSec);
    m_endSpin->blockSignals(false);

    if (m_player) {
        m_player->setMarkRange(m_config.startSec, m_endSpin->value());
    }
    updateEstimateBanner();
}

void GifStudioDialog::onDurationSpinChanged(double val)
{
    m_config.durationSec = val;
    m_endSpin->blockSignals(true);
    m_endSpin->setValue(m_config.startSec + m_config.durationSec);
    m_endSpin->blockSignals(false);

    if (m_player) {
        m_player->setMarkRange(m_config.startSec, m_endSpin->value());
    }
    updateEstimateBanner();
}

void GifStudioDialog::onEndSpinChanged(double val)
{
    if (val > m_config.startSec) {
        m_config.durationSec = val - m_config.startSec;
        m_durationSpin->blockSignals(true);
        m_durationSpin->setValue(m_config.durationSec);
        m_durationSpin->blockSignals(false);
    }
    if (m_player) {
        m_player->setMarkRange(m_config.startSec, val);
    }
    updateEstimateBanner();
}

void GifStudioDialog::onFormatChanged(int index)
{
    m_config.format = static_cast<Animation::OutputFormat>(index);
    if (m_config.format == Animation::OutputFormat::GIF) {
        m_gifPaletteWidget->show();
        m_webpWidget->hide();
    } else if (m_config.format == Animation::OutputFormat::WebP) {
        m_gifPaletteWidget->hide();
        m_webpWidget->show();
    } else {
        m_gifPaletteWidget->hide();
        m_webpWidget->hide();
    }
    updateEstimateBanner();
}

void GifStudioDialog::onPresetChanged(int index)
{
    switch (index) {
    case 0: // HQ 256
        m_colorsSpin->setValue(256);
        m_fpsCombo->setCurrentIndex(2); // 15 fps
        m_ditherCombo->setCurrentIndex(0); // Bayer
        m_bayerSlider->setValue(3);
        m_statsModeCombo->setCurrentIndex(0); // Diff
        break;
    case 1: // Balanced 128
        m_colorsSpin->setValue(128);
        m_fpsCombo->setCurrentIndex(1); // 12 fps
        m_ditherCombo->setCurrentIndex(0);
        m_bayerSlider->setValue(4);
        m_statsModeCombo->setCurrentIndex(0);
        break;
    case 2: // Sticker 64
        m_colorsSpin->setValue(64);
        m_fpsCombo->setCurrentIndex(0); // 10 fps
        m_ditherCombo->setCurrentIndex(3); // None
        m_statsModeCombo->setCurrentIndex(0);
        m_resPresetCombo->setCurrentIndex(2); // 240px
        break;
    case 3: // Smooth 24fps
        m_colorsSpin->setValue(256);
        m_fpsCombo->setCurrentIndex(4); // 24 fps
        m_ditherCombo->setCurrentIndex(0);
        m_bayerSlider->setValue(2);
        m_statsModeCombo->setCurrentIndex(1); // Full
        break;
    }
}

void GifStudioDialog::onResolutionPresetChanged(int /*index*/)
{
    const int val = m_resPresetCombo->currentData().toInt();
    if (val > 0) {
        m_widthSpin->setValue(val);
    }
}

void GifStudioDialog::onWidthChanged(int val)
{
    m_config.width = val;
    updateEstimateBanner();
}

void GifStudioDialog::onFpsChanged(int /*index*/)
{
    m_config.fps = m_fpsCombo->currentData().toInt();
    updateEstimateBanner();
}

void GifStudioDialog::onSpeedChanged(int /*index*/)
{
    m_config.speedMultiplier = m_speedCombo->currentData().toDouble();
    updateEstimateBanner();
}

void GifStudioDialog::onDirectionChanged(int index)
{
    m_config.direction = static_cast<Animation::PlaybackDirection>(index);
    updateEstimateBanner();
}

void GifStudioDialog::onMaxColorsChanged(int val)
{
    m_config.maxColors = val;
    updateEstimateBanner();
}

void GifStudioDialog::onDitherModeChanged(int index)
{
    m_config.dither = static_cast<Animation::DitherMode>(index);
    m_bayerSlider->setEnabled(m_config.dither == Animation::DitherMode::Bayer);
}

void GifStudioDialog::onBayerScaleChanged(int val)
{
    m_config.bayerScale = val;
    m_bayerLabel->setText(QString::number(val));
}

void GifStudioDialog::onStatsModeChanged(int index)
{
    m_config.statsMode = static_cast<Animation::PaletteStatsMode>(index);
}

void GifStudioDialog::onLoopCountChanged(int val)
{
    m_config.loopCount = val;
}

void GifStudioDialog::onWebpQualityChanged(int val)
{
    m_config.webpQuality = val;
    m_webpQualityLabel->setText(QString::number(val));
    updateEstimateBanner();
}

void GifStudioDialog::onWebpLosslessToggled(bool checked)
{
    m_config.webpLossless = checked;
    m_webpQualitySlider->setEnabled(!checked);
    updateEstimateBanner();
}

void GifStudioDialog::updateEstimateBanner()
{
    const int frames = m_config.totalFrames();
    const qint64 bytes = m_config.estimateFileSizeBytes();
    const double mb = bytes / (1024.0 * 1024.0);

    const QString text = QStringLiteral("📊 %1: %2s  |  %3: %4 %5  |  %6: ~%7 MB")
                             .arg(trId("animation.duration"))
                             .arg(m_config.durationSec, 0, 'f', 2)
                             .arg(trId("animation.totalFrames"))
                             .arg(frames)
                             .arg(trId("animation.framesUnit"))
                             .arg(trId("animation.estSize"))
                             .arg(mb, 0, 'f', 2);
    m_estimateBanner->setText(text);
}

void GifStudioDialog::onCopyCommand()
{
    if (m_videoPath.isEmpty()) {
        QMessageBox::warning(this, trId("common.warning"), trId("animation.missingInputNotice"));
        return;
    }

    const QString ext = (m_config.format == Animation::OutputFormat::WebP) ? QStringLiteral("webp")
                      : (m_config.format == Animation::OutputFormat::APNG) ? QStringLiteral("png")
                      : QStringLiteral("gif");
    const QString output = OutputNaming::build(m_videoPath, QStringLiteral("{name}.{format}"), ext);
    const QStringList args = Animation::AnimatedImageEngine::buildCommand(m_videoPath, output, m_config);

    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QString fullCmd = ffmpegPath + QStringLiteral(" ") + args.join(QStringLiteral(" "));

    QGuiApplication::clipboard()->setText(fullCmd);
    QMessageBox::information(this, trId("common.success"), trId("animation.cmdCopiedNotice"));
}

void GifStudioDialog::onAddToQueue()
{
    if (m_videoPath.isEmpty()) {
        QMessageBox::warning(this, trId("common.warning"), trId("animation.missingInputNotice"));
        return;
    }

    if (!m_queue) {
        return;
    }

    const QString ext = (m_config.format == Animation::OutputFormat::WebP) ? QStringLiteral("webp")
                      : (m_config.format == Animation::OutputFormat::APNG) ? QStringLiteral("png")
                      : QStringLiteral("gif");
    const QString output = OutputNaming::build(m_videoPath, QStringLiteral("{name}.{format}"), ext);
    const QStringList args = Animation::AnimatedImageEngine::buildCommand(m_videoPath, output, m_config);

    auto *job = new Job();
    job->setName(trId("animation.job.title").arg(QFileInfo(m_videoPath).fileName()));
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    job->setCommand(ffmpegPath, args);
    m_queue->addJob(job);

    QMessageBox::information(this, trId("common.success"), trId("animation.enqueuedNotice"));
}
