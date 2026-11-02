#include "VideoPlayerWidget.h"

#include <QAudioOutput>
#include <QBoxLayout>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QTimer>
#include <QVideoWidget>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaSlider.h"
#include "ElaText.h"
#include "i18n/TrId.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    // 1. Video display
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumHeight(240);
    m_videoWidget->setStyleSheet(QStringLiteral("background-color: black; border-radius: 6px;"));
    m_videoWidget->installEventFilter(this);
    m_videoWidget->setMouseTracking(true);
    m_player->setVideoOutput(m_videoWidget);

    rootLayout->addWidget(m_videoWidget, 1);

    // 2. Timeline Slider
    m_timeline = new DamageTimelineSlider(this);
    m_timeline->setRange(0, 0);
    connect(m_timeline, &ElaSlider::sliderMoved, this, &VideoPlayerWidget::onSliderMoved);
    rootLayout->addWidget(m_timeline);

    // 3. Control bar container
    m_controlBarWidget = new QWidget(this);
    auto *ctrlLayout = new QHBoxLayout(m_controlBarWidget);
    ctrlLayout->setContentsMargins(4, 0, 4, 0);
    ctrlLayout->setSpacing(6);

    // Play & Stop
    m_playButton = new ElaPushButton(QStringLiteral("▶"), m_controlBarWidget);
    m_playButton->setFixedSize(32, 28);
    m_playButton->setToolTip(trId("player.playPause"));
    connect(m_playButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::togglePlayPause);
    ctrlLayout->addWidget(m_playButton);

    m_stopButton = new ElaPushButton(QStringLiteral("■"), m_controlBarWidget);
    m_stopButton->setFixedSize(32, 28);
    m_stopButton->setToolTip(trId("player.stop"));
    connect(m_stopButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::stop);
    ctrlLayout->addWidget(m_stopButton);

    // Frame stepping and keyframe jump buttons
    m_prevKeyframeBtn = new ElaPushButton(QStringLiteral("|◀"), m_controlBarWidget);
    m_prevKeyframeBtn->setFixedSize(32, 28);
    m_prevKeyframeBtn->setToolTip(trId("player.prevKeyframe"));
    connect(m_prevKeyframeBtn, &ElaPushButton::clicked, this, &VideoPlayerWidget::jumpToPreviousKeyframe);
    ctrlLayout->addWidget(m_prevKeyframeBtn);

    m_stepBackBtn = new ElaPushButton(QStringLiteral("◀|"), m_controlBarWidget);
    m_stepBackBtn->setFixedSize(32, 28);
    m_stepBackBtn->setToolTip(trId("player.stepBackward"));
    connect(m_stepBackBtn, &ElaPushButton::clicked, this, [this]() { stepBackward(0.04); });
    ctrlLayout->addWidget(m_stepBackBtn);

    m_stepForwardBtn = new ElaPushButton(QStringLiteral("|▶"), m_controlBarWidget);
    m_stepForwardBtn->setFixedSize(32, 28);
    m_stepForwardBtn->setToolTip(trId("player.stepForward"));
    connect(m_stepForwardBtn, &ElaPushButton::clicked, this, [this]() { stepForward(0.04); });
    ctrlLayout->addWidget(m_stepForwardBtn);

    m_nextKeyframeBtn = new ElaPushButton(QStringLiteral("▶|"), m_controlBarWidget);
    m_nextKeyframeBtn->setFixedSize(32, 28);
    m_nextKeyframeBtn->setToolTip(trId("player.nextKeyframe"));
    connect(m_nextKeyframeBtn, &ElaPushButton::clicked, this, &VideoPlayerWidget::jumpToNextKeyframe);
    ctrlLayout->addWidget(m_nextKeyframeBtn);

    // Time Label
    m_timeLabel = new ElaText(QStringLiteral("00:00 / 00:00"), 12, m_controlBarWidget);
    ctrlLayout->addWidget(m_timeLabel);

    ctrlLayout->addStretch();

    // Playback Speed Selector
    m_speedCombo = new ElaComboBox(m_controlBarWidget);
    m_speedCombo->setFixedWidth(75);
    m_speedCombo->addItem(QStringLiteral("0.5x"));
    m_speedCombo->addItem(QStringLiteral("0.75x"));
    m_speedCombo->addItem(QStringLiteral("1.0x"));
    m_speedCombo->addItem(QStringLiteral("1.25x"));
    m_speedCombo->addItem(QStringLiteral("1.5x"));
    m_speedCombo->addItem(QStringLiteral("2.0x"));
    m_speedCombo->setCurrentIndex(2); // 1.0x
    connect(m_speedCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &VideoPlayerWidget::onSpeedComboChanged);
    ctrlLayout->addWidget(m_speedCombo);

    // Volume & Mute
    m_muteBtn = new ElaPushButton(QStringLiteral("🔊"), m_controlBarWidget);
    m_muteBtn->setFixedSize(32, 28);
    m_muteBtn->setToolTip(trId("player.muteToggle"));
    connect(m_muteBtn, &ElaPushButton::clicked, this, &VideoPlayerWidget::onMuteClicked);
    ctrlLayout->addWidget(m_muteBtn);

    m_volumeSlider = new ElaSlider(Qt::Horizontal, m_controlBarWidget);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(70);
    m_volumeSlider->setToolTip(trId("player.volume"));
    connect(m_volumeSlider, &ElaSlider::valueChanged, this, &VideoPlayerWidget::onVolumeSliderChanged);
    ctrlLayout->addWidget(m_volumeSlider);

    // Fullscreen Toggle Button
    m_fullscreenBtn = new ElaPushButton(QStringLiteral("⛶"), m_controlBarWidget);
    m_fullscreenBtn->setFixedSize(32, 28);
    m_fullscreenBtn->setToolTip(trId("player.fullscreen"));
    connect(m_fullscreenBtn, &ElaPushButton::clicked, this, &VideoPlayerWidget::toggleFullscreen);
    ctrlLayout->addWidget(m_fullscreenBtn);

    m_skipStatusLabel = new ElaText(QString(), 12, m_controlBarWidget);
    m_skipStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800; font-weight: bold;"));
    ctrlLayout->addWidget(m_skipStatusLabel);

    m_autoSkipCheck = new ElaCheckBox(trId("player.autoSkipDamaged"), m_controlBarWidget);
    m_autoSkipCheck->setChecked(true);
    ctrlLayout->addWidget(m_autoSkipCheck);

    rootLayout->addWidget(m_controlBarWidget);

    // Placeholder widget used to preserve position when going fullscreen
    m_placeholderWidget = new QWidget(this);
    m_placeholderWidget->hide();

    // Idle timer for auto-hiding controls in fullscreen
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &VideoPlayerWidget::onIdleTimeout);

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayerWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayerWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoPlayerWidget::onPlaybackStateChanged);
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    stop();
    if (m_isFullscreen) {
        toggleFullscreen();
    }
}

void VideoPlayerWidget::loadMedia(const QString &filePath)
{
    stop();
    clearSegments();
    clearKeyframes();
    clearCutSegments();
    m_player->setSource(QUrl::fromLocalFile(filePath));
}

void VideoPlayerWidget::setCorruptedSegments(const QVector<CorruptedSegment> &segments)
{
    m_segments = segments;
    m_timeline->setSegments(segments);
}

void VideoPlayerWidget::clearSegments()
{
    m_segments.clear();
    m_timeline->clearSegments();
    m_skipStatusLabel->clear();
}

void VideoPlayerWidget::setMarkRange(double inSec, double outSec)
{
    m_timeline->setMarkRange(inSec, outSec);
}

void VideoPlayerWidget::clearMarkRange()
{
    m_timeline->clearMarkRange();
}

void VideoPlayerWidget::setKeyframes(const QVector<double> &keyframes)
{
    m_timeline->setKeyframes(keyframes);
}

QVector<double> VideoPlayerWidget::keyframes() const
{
    return m_timeline ? m_timeline->keyframes() : QVector<double>();
}

void VideoPlayerWidget::clearKeyframes()
{
    m_timeline->clearKeyframes();
}

void VideoPlayerWidget::setCutSegments(const QVector<QPair<double, double>> &segments)
{
    m_timeline->setCutSegments(segments);
}

void VideoPlayerWidget::clearCutSegments()
{
    m_timeline->clearCutSegments();
}

double VideoPlayerWidget::currentPositionSec() const
{
    return m_player->position() / 1000.0;
}

double VideoPlayerWidget::durationSec() const
{
    return m_durationMs / 1000.0;
}

qint64 VideoPlayerWidget::currentPositionMs() const
{
    return static_cast<qint64>(currentPositionSec() * 1000.0);
}

void VideoPlayerWidget::seekToMs(qint64 ms)
{
    seekToSec(ms / 1000.0);
}

void VideoPlayerWidget::seekToSec(double sec)
{
    const qint64 ms = static_cast<qint64>(sec * 1000.0);
    m_player->setPosition(ms);
    m_timeline->setValue(static_cast<int>(ms));
}

void VideoPlayerWidget::stepForward(double stepSec)
{
    const double target = Media::PlayerNavigationHelper::stepForwardSec(currentPositionSec(), durationSec(), stepSec);
    seekToSec(target);
}

void VideoPlayerWidget::stepBackward(double stepSec)
{
    const double target = Media::PlayerNavigationHelper::stepBackwardSec(currentPositionSec(), stepSec);
    seekToSec(target);
}

void VideoPlayerWidget::jumpToNextKeyframe()
{
    const QVector<double> kfs = keyframes();
    const double nextKf = Media::PlayerNavigationHelper::findNextKeyframe(kfs, currentPositionSec());
    if (nextKf >= 0.0) {
        seekToSec(nextKf);
    }
}

void VideoPlayerWidget::jumpToPreviousKeyframe()
{
    const QVector<double> kfs = keyframes();
    const double prevKf = Media::PlayerNavigationHelper::findPreviousKeyframe(kfs, currentPositionSec());
    if (prevKf >= 0.0) {
        seekToSec(prevKf);
    }
}

void VideoPlayerWidget::setPlaybackRate(double rate)
{
    m_player->setPlaybackRate(rate);
    const int idx = Media::PlayerNavigationHelper::rateToSpeedIndex(rate);
    m_speedCombo->blockSignals(true);
    m_speedCombo->setCurrentIndex(idx);
    m_speedCombo->blockSignals(false);
}

double VideoPlayerWidget::playbackRate() const
{
    return m_player->playbackRate();
}

void VideoPlayerWidget::setVolume(int volumePercent)
{
    const int clamped = qBound(0, volumePercent, 100);
    m_volumeSlider->setValue(clamped);
}

int VideoPlayerWidget::volume() const
{
    return m_volumeSlider->value();
}

void VideoPlayerWidget::setMuted(bool muted)
{
    m_audioOutput->setMuted(muted);
    updateVolumeUi();
}

bool VideoPlayerWidget::isMuted() const
{
    return m_audioOutput->isMuted();
}

void VideoPlayerWidget::onSpeedComboChanged(int index)
{
    const double rate = Media::PlayerNavigationHelper::speedIndexToRate(index);
    m_player->setPlaybackRate(rate);
}

void VideoPlayerWidget::onVolumeSliderChanged(int value)
{
    m_audioOutput->setVolume(Media::PlayerNavigationHelper::volumeToScalar(value));
    if (value > 0 && m_audioOutput->isMuted()) {
        m_audioOutput->setMuted(false);
    }
    updateVolumeUi();
}

void VideoPlayerWidget::onMuteClicked()
{
    m_audioOutput->setMuted(!m_audioOutput->isMuted());
    updateVolumeUi();
}

void VideoPlayerWidget::updateVolumeUi()
{
    if (m_audioOutput->isMuted() || m_volumeSlider->value() == 0) {
        m_muteBtn->setText(QStringLiteral("🔇"));
    } else {
        m_muteBtn->setText(QStringLiteral("🔊"));
    }
}

void VideoPlayerWidget::setAutoSkipVisible(bool visible)
{
    m_autoSkipCheck->setVisible(visible);
    m_skipStatusLabel->setVisible(visible);
}

bool VideoPlayerWidget::isAutoSkipEnabled() const
{
    return m_autoSkipCheck->isChecked();
}

void VideoPlayerWidget::setAutoSkipEnabled(bool enabled)
{
    m_autoSkipCheck->setChecked(enabled);
}

void VideoPlayerWidget::play()
{
    m_player->play();
}

void VideoPlayerWidget::pause()
{
    m_player->pause();
}

void VideoPlayerWidget::stop()
{
    m_player->stop();
    m_player->setPosition(0);
    m_timeline->setValue(0);
}

void VideoPlayerWidget::retranslateUi()
{
    m_autoSkipCheck->setText(trId("player.autoSkipDamaged"));
    m_playButton->setToolTip(trId("player.playPause"));
    m_stopButton->setToolTip(trId("player.stop"));
    m_prevKeyframeBtn->setToolTip(trId("player.prevKeyframe"));
    m_stepBackBtn->setToolTip(trId("player.stepBackward"));
    m_stepForwardBtn->setToolTip(trId("player.stepForward"));
    m_nextKeyframeBtn->setToolTip(trId("player.nextKeyframe"));
    m_muteBtn->setToolTip(trId("player.muteToggle"));
    m_volumeSlider->setToolTip(trId("player.volume"));
    m_fullscreenBtn->setToolTip(m_isFullscreen ? trId("player.exitFullscreen") : trId("player.fullscreen"));
}

void VideoPlayerWidget::toggleFullscreen()
{
    if (!m_isFullscreen) {
        m_normalParent = parentWidget();
        m_normalGeometry = geometry();

        if (m_normalParent && m_normalParent->layout()) {
            auto *box = qobject_cast<QBoxLayout *>(m_normalParent->layout());
            if (box) {
                const int idx = box->indexOf(this);
                if (idx >= 0) {
                    m_placeholderWidget->setParent(m_normalParent);
                    box->insertWidget(idx, m_placeholderWidget);
                    box->removeWidget(this);
                }
            }
        }

        setParent(nullptr);
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        showFullScreen();

        m_isFullscreen = true;
        m_fullscreenBtn->setText(QStringLiteral("🗗"));
        m_fullscreenBtn->setToolTip(trId("player.exitFullscreen"));

        emit fullscreenToggled(true);
        setFocus();
        m_idleTimer->start(2500);
    } else {
        m_idleTimer->stop();
        unsetCursor();
        showNormal();
        setWindowFlags(Qt::Widget);

        if (m_normalParent) {
            auto *box = qobject_cast<QBoxLayout *>(m_normalParent->layout());
            if (box && m_placeholderWidget->parent() == m_normalParent) {
                const int idx = box->indexOf(m_placeholderWidget);
                if (idx >= 0) {
                    box->insertWidget(idx, this);
                    box->removeWidget(m_placeholderWidget);
                    m_placeholderWidget->setParent(this);
                }
            } else if (box) {
                box->addWidget(this);
            }
            setParent(m_normalParent);
        }

        show();
        m_controlBarWidget->show();
        m_timeline->show();

        m_isFullscreen = false;
        m_fullscreenBtn->setText(QStringLiteral("⛶"));
        m_fullscreenBtn->setToolTip(trId("player.fullscreen"));

        emit fullscreenToggled(false);
    }
}

void VideoPlayerWidget::onIdleTimeout()
{
    if (m_isFullscreen && m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_controlBarWidget->hide();
        m_timeline->hide();
        setCursor(Qt::BlankCursor);
    }
}

void VideoPlayerWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if (m_isFullscreen) {
        if (!m_controlBarWidget->isVisible()) {
            m_controlBarWidget->show();
            m_timeline->show();
            unsetCursor();
        }
        m_idleTimer->start(2500);
    }
}

bool VideoPlayerWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_videoWidget) {
        if (event->type() == QEvent::MouseButtonPress) {
            setFocus();
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            toggleFullscreen();
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            mouseMoveEvent(static_cast<QMouseEvent *>(event));
        }
    }
    return QWidget::eventFilter(obj, event);
}

void VideoPlayerWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        togglePlayPause();
        event->accept();
        return;
    case Qt::Key_K:
        pause();
        event->accept();
        return;
    case Qt::Key_J:
        seekToSec(currentPositionSec() - 2.0);
        event->accept();
        return;
    case Qt::Key_L:
        seekToSec(currentPositionSec() + 2.0);
        event->accept();
        return;
    case Qt::Key_Left:
        if (event->modifiers() & Qt::ControlModifier) {
            jumpToPreviousKeyframe();
        } else if (event->modifiers() & Qt::ShiftModifier) {
            seekToSec(currentPositionSec() - 1.0);
        } else {
            stepBackward(0.04);
        }
        event->accept();
        return;
    case Qt::Key_Right:
        if (event->modifiers() & Qt::ControlModifier) {
            jumpToNextKeyframe();
        } else if (event->modifiers() & Qt::ShiftModifier) {
            seekToSec(currentPositionSec() + 1.0);
        } else {
            stepForward(0.04);
        }
        event->accept();
        return;
    case Qt::Key_Up:
        setVolume(volume() + 5);
        event->accept();
        return;
    case Qt::Key_Down:
        setVolume(volume() - 5);
        event->accept();
        return;
    case Qt::Key_M:
        onMuteClicked();
        event->accept();
        return;
    case Qt::Key_F:
    case Qt::Key_F11:
        toggleFullscreen();
        event->accept();
        return;
    case Qt::Key_Escape:
        if (m_isFullscreen) {
            toggleFullscreen();
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void VideoPlayerWidget::onPositionChanged(qint64 positionMs)
{
    if (!m_isSeeking) {
        m_timeline->setValue(static_cast<int>(positionMs));
    }
    m_timeLabel->setText(QStringLiteral("%1 / %2")
                             .arg(formatTime(positionMs))
                             .arg(formatTime(m_durationMs)));
    emit positionChanged(positionMs);

    // Auto-skip logic
    if (m_autoSkipCheck->isChecked() && !m_segments.isEmpty()) {
        const double curSec = positionMs / 1000.0;
        for (const auto &seg : m_segments) {
            if (curSec >= seg.start && curSec < seg.end) {
                const qint64 targetMs = static_cast<qint64>(seg.end * 1000.0) + 100;
                m_player->setPosition(targetMs);
                m_skipStatusLabel->setText(trId("player.skippedMsg").arg(QString::number(seg.end - seg.start, 'f', 1)));
                emit skippedSegment(seg.start, seg.end);
                break;
            }
        }
    }
}

void VideoPlayerWidget::onDurationChanged(qint64 durationMs)
{
    m_durationMs = durationMs;
    m_timeline->setRange(0, static_cast<int>(durationMs));
    m_timeline->setDuration(durationMs / 1000.0);
    m_timeLabel->setText(QStringLiteral("%1 / %2")
                             .arg(formatTime(m_player->position()))
                             .arg(formatTime(m_durationMs)));
}

void VideoPlayerWidget::onPlaybackStateChanged()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_playButton->setText(QStringLiteral("⏸"));
    } else {
        m_playButton->setText(QStringLiteral("▶"));
    }
}

void VideoPlayerWidget::onSliderMoved(int positionMs)
{
    m_isSeeking = true;
    m_player->setPosition(positionMs);
    m_isSeeking = false;
}

void VideoPlayerWidget::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        pause();
    } else {
        play();
    }
}

QString VideoPlayerWidget::formatTime(qint64 ms)
{
    return Media::PlayerNavigationHelper::formatTime(ms, false);
}
