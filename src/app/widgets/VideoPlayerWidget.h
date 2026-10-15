#pragma once

#include "DamageTimelineSlider.h"
#include "media/CorruptedVideoScanner.h"

#include <QWidget>

class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class ElaPushButton;
class ElaCheckBox;
class ElaText;

class VideoPlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayerWidget(QWidget *parent = nullptr);
    ~VideoPlayerWidget() override;

    void loadMedia(const QString &filePath);
    void setCorruptedSegments(const QVector<CorruptedSegment> &segments);
    void clearSegments();

    bool isAutoSkipEnabled() const;
    void setAutoSkipEnabled(bool enabled);

    void play();
    void pause();
    void stop();

    void retranslateUi();

signals:
    void skippedSegment(double fromSec, double toSec);

private slots:
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackStateChanged();
    void onSliderMoved(int positionMs);
    void togglePlayPause();

private:
    static QString formatTime(qint64 ms);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QWidget *m_placeholderWidget = nullptr;

    DamageTimelineSlider *m_timeline = nullptr;
    ElaPushButton *m_playButton = nullptr;
    ElaPushButton *m_stopButton = nullptr;
    ElaCheckBox *m_autoSkipCheck = nullptr;
    ElaText *m_timeLabel = nullptr;
    ElaText *m_skipStatusLabel = nullptr;

    QVector<CorruptedSegment> m_segments;
    qint64 m_durationMs = 0;
    bool m_isSeeking = false;
};
