#include "VideoPlayerWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "i18n/TrId.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Video display
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumHeight(240);
    m_videoWidget->setStyleSheet(QStringLiteral("background-color: black; border-radius: 6px;"));
    m_player->setVideoOutput(m_videoWidget);

    layout->addWidget(m_videoWidget, 1);

    // Timeline Slider
    m_timeline = new DamageTimelineSlider(this);
    m_timeline->setRange(0, 0);
    connect(m_timeline, &ElaSlider::sliderMoved, this, &VideoPlayerWidget::onSliderMoved);
    layout->addWidget(m_timeline);

    // Control bar
    auto *ctrlLayout = new QHBoxLayout();
    ctrlLayout->setContentsMargins(4, 0, 4, 0);
    ctrlLayout->setSpacing(10);

    m_playButton = new ElaPushButton(QStringLiteral("▶"), this);
    m_playButton->setFixedSize(36, 28);
    connect(m_playButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::togglePlayPause);
    ctrlLayout->addWidget(m_playButton);

    m_stopButton = new ElaPushButton(QStringLiteral("■"), this);
    m_stopButton->setFixedSize(36, 28);
    connect(m_stopButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::stop);
    ctrlLayout->addWidget(m_stopButton);

    m_timeLabel = new ElaText(QStringLiteral("00:00 / 00:00"), 12, this);
    ctrlLayout->addWidget(m_timeLabel);

    ctrlLayout->addStretch();

    m_skipStatusLabel = new ElaText(QString(), 12, this);
    m_skipStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800; font-weight: bold;"));
    ctrlLayout->addWidget(m_skipStatusLabel);

    m_autoSkipCheck = new ElaCheckBox(trId("player.autoSkipDamaged"), this);
    m_autoSkipCheck->setChecked(true);
    ctrlLayout->addWidget(m_autoSkipCheck);

    layout->addLayout(ctrlLayout);

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayerWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayerWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoPlayerWidget::onPlaybackStateChanged);
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    stop();
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
    const int totalSeconds = static_cast<int>(ms / 1000);
    const int seconds = totalSeconds % 60;
    const int minutes = (totalSeconds / 60) % 60;
    const int hours = totalSeconds / 3600;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
