#include "VideoPlayerWidget.h"

#include "i18n/TrId.h"

#include <QAudioOutput>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

#include "ElaCheckBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_placeholderWidget(new QWidget(this))
{
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);
    m_videoWidget->setMinimumSize(480, 270);
    m_videoWidget->setStyleSheet(QStringLiteral("background-color: black; border-radius: 8px;"));
    m_videoWidget->setVisible(false);

    m_placeholderWidget->setMinimumSize(480, 270);
    m_placeholderWidget->setStyleSheet(QStringLiteral("background-color: #121212; border: 1px dashed rgba(128, 128, 128, 0.3); border-radius: 8px;"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    root->addWidget(m_placeholderWidget, 1);
    root->addWidget(m_videoWidget, 1);

    m_timeline = new DamageTimelineSlider(Qt::Horizontal, this);
    m_timeline->setRange(0, 0);
    root->addWidget(m_timeline);

    auto *controls = new QHBoxLayout();
    controls->setSpacing(10);

    m_playButton = new ElaPushButton(this);
    m_stopButton = new ElaPushButton(this);
    m_timeLabel = new ElaText(QStringLiteral("00:00 / 00:00"), 12, this);
    m_autoSkipCheck = new ElaCheckBox(this);
    m_autoSkipCheck->setChecked(true);

    m_skipStatusLabel = new ElaText(QString(), 12, this);
    m_skipStatusLabel->setStyleSheet(QStringLiteral("color: #eb4034; font-weight: bold;"));

    controls->addWidget(m_playButton);
    controls->addWidget(m_stopButton);
    controls->addWidget(m_timeLabel);
    controls->addWidget(m_autoSkipCheck);
    controls->addWidget(m_skipStatusLabel);
    controls->addStretch();

    root->addLayout(controls);

    connect(m_playButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::togglePlayPause);
    connect(m_stopButton, &ElaPushButton::clicked, this, &VideoPlayerWidget::stop);
    connect(m_timeline, &ElaSlider::sliderMoved, this, &VideoPlayerWidget::onSliderMoved);
    connect(m_timeline, &ElaSlider::valueChanged, this, [this](int value) {
        if (m_timeline->isSliderDown()) {
            m_player->setPosition(value);
        }
    });

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayerWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayerWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoPlayerWidget::onPlaybackStateChanged);

    retranslateUi();
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    m_player->stop();
}

void VideoPlayerWidget::loadMedia(const QString &filePath)
{
    m_player->stop();
    m_skipStatusLabel->clear();
    m_placeholderWidget->setVisible(false);
    m_videoWidget->setVisible(true);
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
    m_timeline->setValue(0);
    m_timeLabel->setText(QStringLiteral("00:00 / ") + formatTime(m_durationMs));
    m_skipStatusLabel->clear();
}

void VideoPlayerWidget::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        pause();
    } else {
        play();
    }
}

void VideoPlayerWidget::onSliderMoved(int positionMs)
{
    m_player->setPosition(positionMs);
}

void VideoPlayerWidget::onPositionChanged(qint64 positionMs)
{
    if (!m_timeline->isSliderDown()) {
        m_timeline->setValue(static_cast<int>(positionMs));
    }

    m_timeLabel->setText(formatTime(positionMs) + QStringLiteral(" / ") + formatTime(m_durationMs));

    // Auto-skip logic
    if (m_autoSkipCheck->isChecked() && m_player->playbackState() == QMediaPlayer::PlayingState) {
        const double currentSec = positionMs / 1000.0;
        for (const auto &seg : m_segments) {
            if (currentSec >= seg.start && currentSec < seg.end) {
                const double targetSec = seg.end + 0.5;
                const qint64 targetMs = static_cast<qint64>(targetSec * 1000.0);
                if (targetMs < m_durationMs) {
                    m_player->setPosition(targetMs);
                    m_skipStatusLabel->setText(
                        QStringLiteral("⏭ Skipped: [%1s - %2s]")
                            .arg(seg.start, 0, 'f', 1)
                            .arg(seg.end, 0, 'f', 1));
                    emit skippedSegment(seg.start, targetSec);
                }
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
    m_timeLabel->setText(formatTime(m_player->position()) + QStringLiteral(" / ") + formatTime(m_durationMs));
}

void VideoPlayerWidget::onPlaybackStateChanged()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_playButton->setText(QStringLiteral("⏸ ") + trId("player.pause"));
    } else {
        m_playButton->setText(QStringLiteral("▶ ") + trId("player.play"));
    }
}

QString VideoPlayerWidget::formatTime(qint64 ms)
{
    const qint64 totalSec = ms / 1000;
    const qint64 m = totalSec / 60;
    const qint64 s = totalSec % 60;
    return QStringLiteral("%1:%2")
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

void VideoPlayerWidget::retranslateUi()
{
    m_stopButton->setText(trId("player.stop"));
    m_autoSkipCheck->setText(trId("player.autoSkip"));
    onPlaybackStateChanged();
}
