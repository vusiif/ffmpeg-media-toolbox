#pragma once

#include "ElaSlider.h"
#include "media/CorruptedVideoScanner.h"

#include <QVector>

class DamageTimelineSlider : public ElaSlider
{
    Q_OBJECT

public:
    explicit DamageTimelineSlider(QWidget *parent = nullptr);
    explicit DamageTimelineSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setDuration(double durationSeconds);
    double duration() const;

    void setSegments(const QVector<CorruptedSegment> &segments);
    QVector<CorruptedSegment> segments() const;
    void clearSegments();

    void setMarkRange(double inSec, double outSec);
    double markIn() const { return m_markIn; }
    double markOut() const { return m_markOut; }
    void clearMarkRange();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    double m_duration = 0.0;
    double m_markIn = 0.0;
    double m_markOut = 0.0;
    QVector<CorruptedSegment> m_segments;
};
