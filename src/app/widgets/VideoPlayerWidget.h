#pragma once

#include "DamageTimelineSlider.h"
#include "media/CorruptedVideoScanner.h"
#include "media/PlayerNavigationHelper.h"

#include <QPointer>
#include <QWidget>

class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class ElaPushButton;
class ElaCheckBox;
class ElaComboBox;
class ElaSlider;
class ElaText;
class QTimer;

class VideoPlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayerWidget(QWidget *parent = nullptr);
    ~VideoPlayerWidget() override;

    void loadMedia(const QString &filePath);
    void setCorruptedSegments(const QVector<CorruptedSegment> &segments);
    void clearSegments();

    void setMarkRange(double inSec, double outSec);
    void clearMarkRange();
    void setKeyframes(const QVector<double> &keyframes);
    QVector<double> keyframes() const;
    void clearKeyframes();
    void setCutSegments(const QVector<QPair<double, double>> &segments);
    void clearCutSegments();

    double currentPositionSec() const;
    qint64 currentPositionMs() const;
    double durationSec() const;
    void seekToSec(double sec);
    void seekToMs(qint64 ms);
    void setAutoSkipVisible(bool visible);

    bool isAutoSkipEnabled() const;
    void setAutoSkipEnabled(bool enabled);

    void play();
    void pause();
    void stop();

    // Professional frame stepping & keyframe navigation
    void stepForward(double stepSec = 0.04);
    void stepBackward(double stepSec = 0.04);
    void jumpToNextKeyframe();
    void jumpToPreviousKeyframe();

    // Playback rate
    void setPlaybackRate(double rate);
    double playbackRate() const;

    // Volume & Mute
    void setVolume(int volumePercent);
    int volume() const;
    void setMuted(bool muted);
    bool isMuted() const;

    // Fullscreen
    void toggleFullscreen();
    bool isFullscreen() const { return m_isFullscreen; }

    void retranslateUi();

signals:
    void skippedSegment(double fromSec, double toSec);
    void positionChanged(qint64 positionMs);
    void fullscreenToggled(bool isFullscreen);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackStateChanged();
    void onSliderMoved(int positionMs);
    void togglePlayPause();

    void onSpeedComboChanged(int index);
    void onVolumeSliderChanged(int value);
    void onMuteClicked();
    void onIdleTimeout();

private:
    static QString formatTime(qint64 ms);
    void updateVolumeUi();

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QPointer<QWidget> m_placeholderWidget;
    QWidget *m_controlBarWidget = nullptr;

    DamageTimelineSlider *m_timeline = nullptr;
    ElaPushButton *m_playButton = nullptr;
    ElaPushButton *m_stopButton = nullptr;

    // Frame & Keyframe navigation
    ElaPushButton *m_prevKeyframeBtn = nullptr;
    ElaPushButton *m_stepBackBtn = nullptr;
    ElaPushButton *m_stepForwardBtn = nullptr;
    ElaPushButton *m_nextKeyframeBtn = nullptr;

    // Speed & Volume
    ElaComboBox *m_speedCombo = nullptr;
    ElaPushButton *m_muteBtn = nullptr;
    ElaSlider *m_volumeSlider = nullptr;

    // Fullscreen & Skip
    ElaPushButton *m_fullscreenBtn = nullptr;
    ElaCheckBox *m_autoSkipCheck = nullptr;
    ElaText *m_timeLabel = nullptr;
    ElaText *m_skipStatusLabel = nullptr;

    QVector<CorruptedSegment> m_segments;
    qint64 m_durationMs = 0;
    bool m_isSeeking = false;

    // Fullscreen state
    bool m_isFullscreen = false;
    QPointer<QWidget> m_normalParent;
    QMetaObject::Connection m_parentDestroyedConn;
    QRect m_normalGeometry;
    QTimer *m_idleTimer = nullptr;
};
