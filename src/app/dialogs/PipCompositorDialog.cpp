#include "PipCompositorDialog.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
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

// ---------------------------------------------------------------------------
// PipPreviewCanvas Implementation
// ---------------------------------------------------------------------------

PipPreviewCanvas::PipPreviewCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(420, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
}

void PipPreviewCanvas::setConfig(const Compositor::CompositorConfig &config)
{
    m_config = config;
    update();
}

QRect PipPreviewCanvas::canvasRect() const
{
    const int pad = 10;
    const int availW = width() - pad * 2;
    const int availH = height() - pad * 2;
    if (availW <= 0 || availH <= 0) {
        return QRect();
    }

    // Default 16:9 canvas
    int targetW = availW;
    int targetH = availW * 9 / 16;
    if (targetH > availH) {
        targetH = availH;
        targetW = availH * 16 / 9;
    }

    const int x = pad + (availW - targetW) / 2;
    const int y = pad + (availH - targetH) / 2;
    return QRect(x, y, targetW, targetH);
}

QRect PipPreviewCanvas::pipRect(const QRect &canvas) const
{
    if (canvas.isEmpty()) return QRect();

    int pipW = qMax(40, static_cast<int>(canvas.width() * m_config.scalePercent));
    int pipH = qMax(25, static_cast<int>(canvas.height() * m_config.scalePercent));

    const int m = qMax(4, m_config.margin * canvas.width() / 1920);

    int x = 0;
    int y = 0;
    switch (m_config.position) {
    case Compositor::PipPosition::TopLeft:
        x = canvas.left() + m;
        y = canvas.top() + m;
        break;
    case Compositor::PipPosition::TopRight:
        x = canvas.right() - m - pipW;
        y = canvas.top() + m;
        break;
    case Compositor::PipPosition::BottomLeft:
        x = canvas.left() + m;
        y = canvas.bottom() - m - pipH;
        break;
    case Compositor::PipPosition::Center:
        x = canvas.left() + (canvas.width() - pipW) / 2;
        y = canvas.top() + (canvas.height() - pipH) / 2;
        break;
    case Compositor::PipPosition::CustomCoordinate:
        x = canvas.left() + m_config.customX * canvas.width() / 1920;
        y = canvas.top() + m_config.customY * canvas.height() / 1080;
        break;
    case Compositor::PipPosition::BottomRight:
    default:
        x = canvas.right() - m - pipW;
        y = canvas.bottom() - m - pipH;
        break;
    }

    return QRect(x, y, pipW, pipH);
}

void PipPreviewCanvas::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect canvas = canvasRect();
    if (canvas.isEmpty()) return;

    // 1. Draw outer background container
    p.fillRect(rect(), QColor(22, 22, 28));

    // 2. Draw canvas frame
    p.setPen(QPen(QColor(60, 60, 75), 1));
    p.setBrush(QColor(32, 33, 42));
    p.drawRoundedRect(canvas, 6, 6);

    if (m_config.layout == Compositor::LayoutMode::PictureInPicture) {
        // Draw crosshair/grid lines on base
        p.setPen(QPen(QColor(50, 52, 65), 1, Qt::DashLine));
        p.drawLine(canvas.center().x(), canvas.top(), canvas.center().x(), canvas.bottom());
        p.drawLine(canvas.left(), canvas.center().y(), canvas.right(), canvas.center().y());

        // Center label for base
        p.setPen(QColor(140, 145, 160));
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(10);
        p.setFont(f);
        p.drawText(canvas, Qt::AlignCenter, trId("compositor.label.main"));

        // Draw PiP overlay
        const QRect pip = pipRect(canvas);
        p.setBrush(QColor(0, 120, 215, 180));
        QColor borderCol(255, 255, 255);
        if (m_config.drawBorder) {
            if (m_config.borderColor.compare(QLatin1String("black"), Qt::CaseInsensitive) == 0) {
                borderCol = QColor(0, 0, 0);
            } else if (m_config.borderColor.compare(QLatin1String("yellow"), Qt::CaseInsensitive) == 0) {
                borderCol = QColor(255, 215, 0);
            } else if (m_config.borderColor.compare(QLatin1String("red"), Qt::CaseInsensitive) == 0) {
                borderCol = QColor(230, 40, 40);
            } else if (m_config.borderColor.compare(QLatin1String("cyan"), Qt::CaseInsensitive) == 0) {
                borderCol = QColor(0, 230, 230);
            }
        }
        p.setPen(QPen(borderCol, m_config.drawBorder ? qBound(1, m_config.borderWidth, 6) : 2));
        p.drawRoundedRect(pip, 4, 4);

        // PiP label
        p.setPen(Qt::white);
        f.setPointSize(8);
        p.setFont(f);
        p.drawText(pip, Qt::AlignCenter, trId("compositor.label.pip"));

    } else if (m_config.layout == Compositor::LayoutMode::SideBySide) {
        const int splitX = canvas.left() + static_cast<int>(canvas.width() * m_config.splitRatio);
        const QRect leftRect(canvas.left(), canvas.top(), splitX - canvas.left(), canvas.height());
        const QRect rightRect(splitX, canvas.top(), canvas.right() - splitX, canvas.height());

        p.setBrush(QColor(28, 30, 40));
        p.setPen(Qt::NoPen);
        p.drawRect(leftRect);

        p.setBrush(QColor(38, 40, 52));
        p.drawRect(rightRect);

        // Dividing line
        p.setPen(QPen(QColor(0, 120, 215), 2));
        p.drawLine(splitX, canvas.top(), splitX, canvas.bottom());

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(9);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(leftRect, Qt::AlignCenter, trId("compositor.label.leftMain"));
        p.drawText(rightRect, Qt::AlignCenter, trId("compositor.label.rightPip"));

    } else if (m_config.layout == Compositor::LayoutMode::TopBottom) {
        const int splitY = canvas.top() + static_cast<int>(canvas.height() * m_config.splitRatio);
        const QRect topRect(canvas.left(), canvas.top(), canvas.width(), splitY - canvas.top());
        const QRect botRect(canvas.left(), splitY, canvas.width(), canvas.bottom() - splitY);

        p.setBrush(QColor(28, 30, 40));
        p.setPen(Qt::NoPen);
        p.drawRect(topRect);

        p.setBrush(QColor(38, 40, 52));
        p.drawRect(botRect);

        // Dividing line
        p.setPen(QPen(QColor(0, 120, 215), 2));
        p.drawLine(canvas.left(), splitY, canvas.right(), splitY);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(9);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(topRect, Qt::AlignCenter, trId("compositor.label.topMain"));
        p.drawText(botRect, Qt::AlignCenter, trId("compositor.label.bottomPip"));
    }
}

void PipPreviewCanvas::mousePressEvent(QMouseEvent *event)
{
    if (m_config.layout != Compositor::LayoutMode::PictureInPicture) {
        return;
    }

    const QRect canvas = canvasRect();
    const QRect pip = pipRect(canvas);

    if (pip.contains(event->pos())) {
        m_dragging = true;
        m_dragOffset = event->pos() - pip.topLeft();
        setCursor(Qt::ClosedHandCursor);
    }
}

void PipPreviewCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_config.layout != Compositor::LayoutMode::PictureInPicture) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    const QRect canvas = canvasRect();
    const QRect pip = pipRect(canvas);

    if (m_dragging) {
        const QPoint newTopLeft = event->pos() - m_dragOffset;
        const int pipW = pip.width();
        const int pipH = pip.height();

        const int boundedX = qBound(canvas.left(), newTopLeft.x(), canvas.right() - pipW);
        const int boundedY = qBound(canvas.top(), newTopLeft.y(), canvas.bottom() - pipH);

        m_config.customX = (boundedX - canvas.left()) * 1920 / canvas.width();
        m_config.customY = (boundedY - canvas.top()) * 1080 / canvas.height();
        m_config.position = Compositor::PipPosition::CustomCoordinate;

        update();
        emit configChanged(m_config);
    } else {
        if (pip.contains(event->pos())) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void PipPreviewCanvas::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    if (m_dragging) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
}

// ---------------------------------------------------------------------------
// PipCompositorDialog Implementation
// ---------------------------------------------------------------------------

PipCompositorDialog::PipCompositorDialog(JobQueue *queue, FFmpegLocator *locator, QWidget *parent)
    : QDialog(parent)
    , m_queue(queue)
    , m_locator(locator)
{
    resize(1120, 780);
    setWindowTitle(trId("compositor.dialog.title"));

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

    // Left Column: Inputs & Visual Canvas & Player
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(10);

    // Input Files Card
    auto *inputCard = new ElaScrollPageArea(this);
    auto *inputLayout = new QFormLayout(inputCard);
    inputLayout->setSpacing(8);

    auto *mainRow = new QHBoxLayout();
    m_mainPathEdit = new ElaLineEdit(inputCard);
    m_mainPathEdit->setReadOnly(true);
    m_mainBrowseBtn = new ElaPushButton(inputCard);
    connect(m_mainBrowseBtn, &ElaPushButton::clicked, this, &PipCompositorDialog::onBrowseMain);
    mainRow->addWidget(m_mainPathEdit, 1);
    mainRow->addWidget(m_mainBrowseBtn);
    inputLayout->addRow(new ElaText(trId("compositor.mainVideo") + QStringLiteral(":"), inputCard), mainRow);

    auto *pipRow = new QHBoxLayout();
    m_pipPathEdit = new ElaLineEdit(inputCard);
    m_pipPathEdit->setReadOnly(true);
    m_pipBrowseBtn = new ElaPushButton(inputCard);
    connect(m_pipBrowseBtn, &ElaPushButton::clicked, this, &PipCompositorDialog::onBrowsePip);
    pipRow->addWidget(m_pipPathEdit, 1);
    pipRow->addWidget(m_pipBrowseBtn);
    inputLayout->addRow(new ElaText(trId("compositor.pipVideo") + QStringLiteral(":"), inputCard), pipRow);

    leftCol->addWidget(inputCard);

    // Canvas Card
    auto *canvasCard = new ElaScrollPageArea(this);
    auto *canvasLayout = new QVBoxLayout(canvasCard);
    canvasLayout->setContentsMargins(10, 10, 10, 10);
    canvasLayout->setSpacing(6);

    m_canvas = new PipPreviewCanvas(canvasCard);
    connect(m_canvas, &PipPreviewCanvas::configChanged, this, &PipCompositorDialog::onCanvasConfigChanged);
    canvasLayout->addWidget(m_canvas);

    auto *tipText = new ElaText(trId("compositor.canvasTip"), canvasCard);
    tipText->setTextPixelSize(12);
    tipText->setStyleSheet(QStringLiteral("color: #888888;"));
    tipText->setAlignment(Qt::AlignCenter);
    canvasLayout->addWidget(tipText);

    leftCol->addWidget(canvasCard);

    // Video Player
    m_player = new VideoPlayerWidget(this);
    m_player->setMinimumHeight(240);
    leftCol->addWidget(m_player, 1);

    bodyLayout->addLayout(leftCol, 5);

    // Right Column: Controls in Scroll Area
    auto *rightScroll = new ElaScrollArea(this);
    rightScroll->setWidgetResizable(true);
    auto *rightWidget = new QWidget(rightScroll);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(12);

    // Card 1: Layout & Modes
    auto *layoutCard = new ElaScrollPageArea(rightWidget);
    auto *layoutForm = new QFormLayout(layoutCard);
    layoutForm->setSpacing(8);

    m_layoutCombo = new ElaComboBox(layoutCard);
    m_layoutCombo->addItem(trId("compositor.mode.pip"));
    m_layoutCombo->addItem(trId("compositor.mode.sbs"));
    m_layoutCombo->addItem(trId("compositor.mode.topbottom"));
    connect(m_layoutCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PipCompositorDialog::onLayoutModeChanged);
    layoutForm->addRow(new ElaText(trId("compositor.layoutMode") + QStringLiteral(":"), layoutCard), m_layoutCombo);

    // PiP Specific Controls Container
    m_pipParamWidget = new QWidget(layoutCard);
    auto *pipParamForm = new QFormLayout(m_pipParamWidget);
    pipParamForm->setContentsMargins(0, 0, 0, 0);
    pipParamForm->setSpacing(8);

    m_positionCombo = new ElaComboBox(m_pipParamWidget);
    m_positionCombo->addItem(trId("video.wm.bottomRight"));
    m_positionCombo->addItem(trId("video.wm.bottomLeft"));
    m_positionCombo->addItem(trId("video.wm.topRight"));
    m_positionCombo->addItem(trId("video.wm.topLeft"));
    m_positionCombo->addItem(trId("video.wm.center"));
    m_positionCombo->addItem(trId("compositor.pos.custom"));
    connect(m_positionCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PipCompositorDialog::onPositionPresetChanged);
    pipParamForm->addRow(new ElaText(trId("common.position") + QStringLiteral(":"), m_pipParamWidget), m_positionCombo);

    auto *scaleRow = new QHBoxLayout();
    m_scaleSlider = new ElaSlider(Qt::Horizontal, m_pipParamWidget);
    m_scaleSlider->setRange(10, 75);
    m_scaleSlider->setValue(25);
    m_scaleLabel = new QLabel(QStringLiteral("25%"), m_pipParamWidget);
    m_scaleLabel->setFixedWidth(40);
    connect(m_scaleSlider, &ElaSlider::valueChanged, this, &PipCompositorDialog::onScaleSliderChanged);
    scaleRow->addWidget(m_scaleSlider, 1);
    scaleRow->addWidget(m_scaleLabel);
    pipParamForm->addRow(new ElaText(trId("compositor.scale") + QStringLiteral(":"), m_pipParamWidget), scaleRow);

    m_marginSpin = new ElaSpinBox(m_pipParamWidget);
    m_marginSpin->setRange(0, 200);
    m_marginSpin->setValue(16);
    connect(m_marginSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, &PipCompositorDialog::onMarginChanged);
    pipParamForm->addRow(new ElaText(trId("video.pip.margin") + QStringLiteral(":"), m_pipParamWidget), m_marginSpin);

    auto *borderRow = new QHBoxLayout();
    m_borderCheck = new ElaCheckBox(trId("compositor.drawBorder"), m_pipParamWidget);
    connect(m_borderCheck, &ElaCheckBox::toggled, this, &PipCompositorDialog::onBorderToggled);
    m_borderWidthSpin = new ElaSpinBox(m_pipParamWidget);
    m_borderWidthSpin->setRange(1, 10);
    m_borderWidthSpin->setValue(2);
    m_borderWidthSpin->setEnabled(false);
    connect(m_borderWidthSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, &PipCompositorDialog::onBorderWidthChanged);

    m_borderColorCombo = new ElaComboBox(m_pipParamWidget);
    m_borderColorCombo->addItem(trId("compositor.color.white"), QStringLiteral("white"));
    m_borderColorCombo->addItem(trId("compositor.color.black"), QStringLiteral("black"));
    m_borderColorCombo->addItem(trId("compositor.color.yellow"), QStringLiteral("yellow"));
    m_borderColorCombo->addItem(trId("compositor.color.cyan"), QStringLiteral("cyan"));
    m_borderColorCombo->addItem(trId("compositor.color.red"), QStringLiteral("red"));
    m_borderColorCombo->setEnabled(false);
    connect(m_borderColorCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PipCompositorDialog::onBorderColorChanged);

    borderRow->addWidget(m_borderCheck);
    borderRow->addWidget(m_borderWidthSpin);
    borderRow->addWidget(m_borderColorCombo);
    pipParamForm->addRow(new ElaText(trId("compositor.border") + QStringLiteral(":"), m_pipParamWidget), borderRow);

    auto *timeRow = new QHBoxLayout();
    m_timeRangeCheck = new ElaCheckBox(trId("compositor.enableTimeWindow"), m_pipParamWidget);
    connect(m_timeRangeCheck, &ElaCheckBox::toggled, this, &PipCompositorDialog::onTimeRangeToggled);
    m_startTimeSpin = new ElaDoubleSpinBox(m_pipParamWidget);
    m_startTimeSpin->setRange(0, 36000);
    m_startTimeSpin->setValue(0);
    m_startTimeSpin->setPrefix(QStringLiteral("从 "));
    m_startTimeSpin->setSuffix(QStringLiteral(" 秒"));
    m_startTimeSpin->setEnabled(false);
    connect(m_startTimeSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &PipCompositorDialog::onStartTimeChanged);

    m_durationSpin = new ElaDoubleSpinBox(m_pipParamWidget);
    m_durationSpin->setRange(0, 36000);
    m_durationSpin->setValue(0);
    m_durationSpin->setPrefix(QStringLiteral("持续 "));
    m_durationSpin->setSuffix(QStringLiteral(" 秒"));
    m_durationSpin->setEnabled(false);
    connect(m_durationSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &PipCompositorDialog::onDurationChanged);

    timeRow->addWidget(m_timeRangeCheck);
    timeRow->addWidget(m_startTimeSpin);
    timeRow->addWidget(m_durationSpin);
    pipParamForm->addRow(new ElaText(trId("compositor.timeRange") + QStringLiteral(":"), m_pipParamWidget), timeRow);

    layoutForm->addRow(m_pipParamWidget);

    // Split Specific Controls Container
    m_splitParamWidget = new QWidget(layoutCard);
    auto *splitParamForm = new QFormLayout(m_splitParamWidget);
    splitParamForm->setContentsMargins(0, 0, 0, 0);
    splitParamForm->setSpacing(8);

    m_resolutionCombo = new ElaComboBox(m_splitParamWidget);
    m_resolutionCombo->addItem(QStringLiteral("1080p (1920x1080)"), QSize(1920, 1080));
    m_resolutionCombo->addItem(QStringLiteral("720p (1280x720)"), QSize(1280, 720));
    m_resolutionCombo->addItem(QStringLiteral("4K UHD (3840x2160)"), QSize(3840, 2160));
    m_resolutionCombo->addItem(QStringLiteral("9:16 短视频 (1080x1920)"), QSize(1080, 1920));
    connect(m_resolutionCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PipCompositorDialog::onResolutionChanged);
    splitParamForm->addRow(new ElaText(trId("compositor.canvasRes") + QStringLiteral(":"), m_splitParamWidget), m_resolutionCombo);

    auto *splitRatioRow = new QHBoxLayout();
    m_splitRatioSlider = new ElaSlider(Qt::Horizontal, m_splitParamWidget);
    m_splitRatioSlider->setRange(20, 80);
    m_splitRatioSlider->setValue(50);
    m_splitRatioLabel = new QLabel(QStringLiteral("50:50"), m_splitParamWidget);
    m_splitRatioLabel->setFixedWidth(50);
    connect(m_splitRatioSlider, &ElaSlider::valueChanged, this, &PipCompositorDialog::onSplitRatioSliderChanged);
    splitRatioRow->addWidget(m_splitRatioSlider, 1);
    splitRatioRow->addWidget(m_splitRatioLabel);
    splitParamForm->addRow(new ElaText(trId("compositor.splitRatio") + QStringLiteral(":"), m_splitParamWidget), splitRatioRow);

    layoutForm->addRow(m_splitParamWidget);
    m_splitParamWidget->hide();

    rightLayout->addWidget(layoutCard);

    // Card 2: Audio Mixing
    auto *audioCard = new ElaScrollPageArea(rightWidget);
    auto *audioForm = new QFormLayout(audioCard);
    audioForm->setSpacing(8);

    m_audioPolicyCombo = new ElaComboBox(audioCard);
    m_audioPolicyCombo->addItem(trId("compositor.audio.mixDual"));
    m_audioPolicyCombo->addItem(trId("compositor.audio.mainOnly"));
    m_audioPolicyCombo->addItem(trId("compositor.audio.pipOnly"));
    m_audioPolicyCombo->addItem(trId("compositor.audio.ducking"));
    m_audioPolicyCombo->addItem(trId("compositor.audio.muteAll"));
    connect(m_audioPolicyCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PipCompositorDialog::onAudioPolicyChanged);
    audioForm->addRow(new ElaText(trId("compositor.audioPolicy") + QStringLiteral(":"), audioCard), m_audioPolicyCombo);

    auto *mainVolRow = new QHBoxLayout();
    m_mainVolumeSlider = new ElaSlider(Qt::Horizontal, audioCard);
    m_mainVolumeSlider->setRange(0, 200);
    m_mainVolumeSlider->setValue(100);
    m_mainVolumeLabel = new QLabel(QStringLiteral("100%"), audioCard);
    m_mainVolumeLabel->setFixedWidth(40);
    connect(m_mainVolumeSlider, &ElaSlider::valueChanged, this, &PipCompositorDialog::onMainVolumeSliderChanged);
    mainVolRow->addWidget(m_mainVolumeSlider, 1);
    mainVolRow->addWidget(m_mainVolumeLabel);
    audioForm->addRow(new ElaText(trId("compositor.mainVol") + QStringLiteral(":"), audioCard), mainVolRow);

    auto *pipVolRow = new QHBoxLayout();
    m_pipVolumeSlider = new ElaSlider(Qt::Horizontal, audioCard);
    m_pipVolumeSlider->setRange(0, 200);
    m_pipVolumeSlider->setValue(80);
    m_pipVolumeLabel = new QLabel(QStringLiteral("80%"), audioCard);
    m_pipVolumeLabel->setFixedWidth(40);
    connect(m_pipVolumeSlider, &ElaSlider::valueChanged, this, &PipCompositorDialog::onPipVolumeSliderChanged);
    pipVolRow->addWidget(m_pipVolumeSlider, 1);
    pipVolRow->addWidget(m_pipVolumeLabel);
    audioForm->addRow(new ElaText(trId("compositor.pipVol") + QStringLiteral(":"), audioCard), pipVolRow);

    rightLayout->addWidget(audioCard);

    // Card 3: Output & Codec
    auto *outputCard = new ElaScrollPageArea(rightWidget);
    auto *outputForm = new QFormLayout(outputCard);
    outputForm->setSpacing(8);

    m_videoCodecCombo = new ElaComboBox(outputCard);
    m_videoCodecCombo->addItem(QStringLiteral("H.264 (libx264)"), QStringLiteral("libx264"));
    m_videoCodecCombo->addItem(QStringLiteral("H.265 / HEVC (libx265)"), QStringLiteral("libx265"));
    outputForm->addRow(new ElaText(trId("convert.codec") + QStringLiteral(":"), outputCard), m_videoCodecCombo);

    m_crfSpin = new ElaSpinBox(outputCard);
    m_crfSpin->setRange(16, 35);
    m_crfSpin->setValue(23);
    outputForm->addRow(new ElaText(trId("convert.quality") + QStringLiteral(" (CRF):"), outputCard), m_crfSpin);

    m_shortestCheck = new ElaCheckBox(trId("compositor.shortest"), outputCard);
    outputForm->addRow(QString(), m_shortestCheck);

    rightLayout->addWidget(outputCard);

    // Action Buttons
    auto *btnLayout = new QHBoxLayout();
    m_copyCmdBtn = new ElaPushButton(trId("compositor.copyCmd"), rightWidget);
    connect(m_copyCmdBtn, &ElaPushButton::clicked, this, &PipCompositorDialog::onCopyCommand);

    m_addToQueueBtn = new ElaPushButton(trId("compositor.addToQueue"), rightWidget);
    m_addToQueueBtn->setStyleSheet(QStringLiteral("background-color: #0078d4; color: white; font-weight: bold;"));
    connect(m_addToQueueBtn, &ElaPushButton::clicked, this, &PipCompositorDialog::onAddToQueue);

    btnLayout->addWidget(m_copyCmdBtn);
    btnLayout->addWidget(m_addToQueueBtn);
    rightLayout->addLayout(btnLayout);

    rightScroll->setWidget(rightWidget);
    bodyLayout->addWidget(rightScroll, 4);

    root->addLayout(bodyLayout);

    retranslateUi();
}

PipCompositorDialog::~PipCompositorDialog() = default;

void PipCompositorDialog::setMainVideo(const QString &mainPath)
{
    m_mainPath = mainPath;
    m_mainPathEdit->setText(mainPath);
    if (!mainPath.isEmpty() && m_player) {
        m_player->loadMedia(mainPath);
    }
}

void PipCompositorDialog::setPipVideo(const QString &pipPath)
{
    m_pipPath = pipPath;
    m_pipPathEdit->setText(pipPath);
}

void PipCompositorDialog::retranslateUi()
{
    setWindowTitle(trId("compositor.dialog.title"));
    m_titleLabel->setText(trId("compositor.dialog.title"));
    m_subtitleLabel->setText(trId("compositor.dialog.subtitle"));
    m_mainBrowseBtn->setText(trId("common.browse"));
    m_pipBrowseBtn->setText(trId("common.browse"));

    m_layoutCombo->setItemText(0, trId("compositor.mode.pip"));
    m_layoutCombo->setItemText(1, trId("compositor.mode.sbs"));
    m_layoutCombo->setItemText(2, trId("compositor.mode.topbottom"));

    m_positionCombo->setItemText(0, trId("video.wm.bottomRight"));
    m_positionCombo->setItemText(1, trId("video.wm.bottomLeft"));
    m_positionCombo->setItemText(2, trId("video.wm.topRight"));
    m_positionCombo->setItemText(3, trId("video.wm.topLeft"));
    m_positionCombo->setItemText(4, trId("video.wm.center"));
    m_positionCombo->setItemText(5, trId("compositor.pos.custom"));

    m_audioPolicyCombo->setItemText(0, trId("compositor.audio.mixDual"));
    m_audioPolicyCombo->setItemText(1, trId("compositor.audio.mainOnly"));
    m_audioPolicyCombo->setItemText(2, trId("compositor.audio.pipOnly"));
    m_audioPolicyCombo->setItemText(3, trId("compositor.audio.ducking"));
    m_audioPolicyCombo->setItemText(4, trId("compositor.audio.muteAll"));

    m_copyCmdBtn->setText(trId("compositor.copyCmd"));
    m_addToQueueBtn->setText(trId("compositor.addToQueue"));
}

void PipCompositorDialog::onBrowseMain()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("compositor.mainVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.ts);;All files (*)"));
    if (!path.isEmpty()) {
        setMainVideo(path);
    }
}

void PipCompositorDialog::onBrowsePip()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("compositor.pipVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.ts);;All files (*)"));
    if (!path.isEmpty()) {
        setPipVideo(path);
    }
}

void PipCompositorDialog::onLayoutModeChanged(int index)
{
    m_config.layout = static_cast<Compositor::LayoutMode>(index);
    if (m_config.layout == Compositor::LayoutMode::PictureInPicture) {
        m_pipParamWidget->show();
        m_splitParamWidget->hide();
    } else {
        m_pipParamWidget->hide();
        m_splitParamWidget->show();
    }
    refreshPreview();
}

void PipCompositorDialog::onPositionPresetChanged(int index)
{
    m_config.position = static_cast<Compositor::PipPosition>(index);
    refreshPreview();
}

void PipCompositorDialog::onScaleSliderChanged(int value)
{
    m_config.scalePercent = value / 100.0;
    m_scaleLabel->setText(QStringLiteral("%1%").arg(value));
    refreshPreview();
}

void PipCompositorDialog::onMarginChanged(int value)
{
    m_config.margin = value;
    refreshPreview();
}

void PipCompositorDialog::onBorderToggled(bool checked)
{
    m_config.drawBorder = checked;
    m_borderWidthSpin->setEnabled(checked);
    m_borderColorCombo->setEnabled(checked);
    refreshPreview();
}

void PipCompositorDialog::onBorderWidthChanged(int value)
{
    m_config.borderWidth = value;
    refreshPreview();
}

void PipCompositorDialog::onBorderColorChanged(int /*index*/)
{
    m_config.borderColor = m_borderColorCombo->currentData().toString();
    refreshPreview();
}

void PipCompositorDialog::onTimeRangeToggled(bool checked)
{
    m_config.hasTimeRange = checked;
    m_startTimeSpin->setEnabled(checked);
    m_durationSpin->setEnabled(checked);
}

void PipCompositorDialog::onStartTimeChanged(double value)
{
    m_config.startTimeSec = value;
}

void PipCompositorDialog::onDurationChanged(double value)
{
    m_config.durationSec = value;
}

void PipCompositorDialog::onResolutionChanged(int /*index*/)
{
    const QSize size = m_resolutionCombo->currentData().toSize();
    if (size.isValid()) {
        m_config.splitWidth = size.width();
        m_config.splitHeight = size.height();
    }
    refreshPreview();
}

void PipCompositorDialog::onSplitRatioSliderChanged(int value)
{
    m_config.splitRatio = value / 100.0;
    m_splitRatioLabel->setText(QStringLiteral("%1:%2").arg(value).arg(100 - value));
    refreshPreview();
}

void PipCompositorDialog::onAudioPolicyChanged(int index)
{
    m_config.audioPolicy = static_cast<Compositor::AudioMixPolicy>(index);
}

void PipCompositorDialog::onMainVolumeSliderChanged(int value)
{
    m_config.mainVolume = value / 100.0;
    m_mainVolumeLabel->setText(QStringLiteral("%1%").arg(value));
}

void PipCompositorDialog::onPipVolumeSliderChanged(int value)
{
    m_config.pipVolume = value / 100.0;
    m_pipVolumeLabel->setText(QStringLiteral("%1%").arg(value));
}

void PipCompositorDialog::onCanvasConfigChanged(const Compositor::CompositorConfig &newConfig)
{
    m_config = newConfig;
    if (m_config.position == Compositor::PipPosition::CustomCoordinate) {
        m_positionCombo->blockSignals(true);
        m_positionCombo->setCurrentIndex(5); // Custom
        m_positionCombo->blockSignals(false);
    }
}

void PipCompositorDialog::refreshPreview()
{
    if (m_canvas) {
        m_canvas->setConfig(m_config);
    }
}

void PipCompositorDialog::syncConfigFromUi()
{
    m_config.videoCodec = m_videoCodecCombo->currentData().toString();
    m_config.crf = m_crfSpin->value();
    m_config.shortest = m_shortestCheck->isChecked();
}

void PipCompositorDialog::onCopyCommand()
{
    if (m_mainPath.isEmpty() || m_pipPath.isEmpty()) {
        QMessageBox::warning(this, trId("common.warning"), trId("compositor.missingInputNotice"));
        return;
    }

    syncConfigFromUi();

    const QString output = OutputNaming::build(
        m_mainPath, QStringLiteral("{name}_composite.{format}"), QFileInfo(m_mainPath).suffix());

    const QStringList args = Compositor::MultiTrackCompositor::buildCommand(
        m_mainPath, m_pipPath, output, m_config);

    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QString fullCmd = ffmpegPath + QStringLiteral(" ") + args.join(QStringLiteral(" "));

    QGuiApplication::clipboard()->setText(fullCmd);
    QMessageBox::information(this, trId("common.success"), trId("compositor.cmdCopiedNotice"));
}

void PipCompositorDialog::onAddToQueue()
{
    if (m_mainPath.isEmpty() || m_pipPath.isEmpty()) {
        QMessageBox::warning(this, trId("common.warning"), trId("compositor.missingInputNotice"));
        return;
    }

    if (!m_queue) {
        return;
    }

    syncConfigFromUi();

    const QString output = OutputNaming::build(
        m_mainPath, QStringLiteral("{name}_composite.{format}"), QFileInfo(m_mainPath).suffix());

    const QStringList args = Compositor::MultiTrackCompositor::buildCommand(
        m_mainPath, m_pipPath, output, m_config);

    auto *job = new Job();
    job->setName(trId("compositor.job.title").arg(QFileInfo(m_mainPath).fileName()));
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    job->setCommand(ffmpegPath, args);
    m_queue->addJob(job);

    QMessageBox::information(this, trId("common.success"), trId("compositor.enqueuedNotice"));
}
