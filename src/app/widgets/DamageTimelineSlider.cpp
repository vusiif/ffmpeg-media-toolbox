#include "DamageTimelineSlider.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

DamageTimelineSlider::DamageTimelineSlider(QWidget *parent)
    : ElaSlider(Qt::Horizontal, parent)
{
    setMouseTracking(true);
}

DamageTimelineSlider::DamageTimelineSlider(Qt::Orientation orientation, QWidget *parent)
    : ElaSlider(orientation, parent)
{
    setMouseTracking(true);
}

void DamageTimelineSlider::setDuration(double durationSeconds)
{
    m_duration = qMax(0.0, durationSeconds);
    update();
}

double DamageTimelineSlider::duration() const
{
    return m_duration;
}

void DamageTimelineSlider::setSegments(const QVector<CorruptedSegment> &segments)
{
    m_segments = segments;
    update();
}

QVector<CorruptedSegment> DamageTimelineSlider::segments() const
{
    return m_segments;
}

void DamageTimelineSlider::clearSegments()
{
    m_segments.clear();
    update();
}

void DamageTimelineSlider::setMarkRange(double inSec, double outSec)
{
    m_markIn = qMax(0.0, inSec);
    m_markOut = qMax(0.0, outSec);
    update();
}

void DamageTimelineSlider::clearMarkRange()
{
    m_markIn = 0.0;
    m_markOut = 0.0;
    update();
}

void DamageTimelineSlider::paintEvent(QPaintEvent *event)
{
    ElaSlider::paintEvent(event);

    if (m_duration <= 0.0) {
        return;
    }

    const int margin = 10;
    const int trackWidth = width() - (margin * 2);
    if (trackWidth <= 0) return;

    const int cy = height() / 2;
    const int barHeight = 8;
    const int barY = cy - (barHeight / 2);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. Paint Trim Selected Range (if any)
    if (m_markOut > m_markIn) {
        const double inFrac = qBound(0.0, m_markIn / m_duration, 1.0);
        const double outFrac = qBound(0.0, m_markOut / m_duration, 1.0);

        const int x1 = margin + static_cast<int>(inFrac * trackWidth);
        const int x2 = margin + static_cast<int>(outFrac * trackWidth);
        const int w = qMax(2, x2 - x1);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 120, 212, 100));
        painter.drawRoundedRect(QRect(x1, barY - 1, w, barHeight + 2), 3, 3);

        painter.setPen(QPen(QColor(0, 120, 212, 230), 2));
        painter.drawLine(x1, barY - 3, x1, barY + barHeight + 3);
        painter.drawLine(x2, barY - 3, x2, barY + barHeight + 3);
    }

    // 2. Paint Corrupted Segments (if any)
    if (!m_segments.isEmpty()) {
        painter.setPen(QPen(QColor(255, 60, 60, 220), 1));
        painter.setBrush(QColor(235, 60, 60, 180));

        for (const auto &seg : m_segments) {
            const double startFrac = qBound(0.0, seg.start / m_duration, 1.0);
            const double endFrac = qBound(0.0, seg.end / m_duration, 1.0);

            const int x1 = margin + static_cast<int>(startFrac * trackWidth);
            const int x2 = margin + static_cast<int>(endFrac * trackWidth);
            const int w = qMax(4, x2 - x1);

            painter.drawRoundedRect(QRect(x1, barY, w, barHeight), 2, 2);
        }
    }
}

void DamageTimelineSlider::mouseMoveEvent(QMouseEvent *event)
{
    ElaSlider::mouseMoveEvent(event);

    if (m_segments.isEmpty() || m_duration <= 0.0) {
        return;
    }

    const int margin = 10;
    const int trackWidth = width() - (margin * 2);
    if (trackWidth <= 0) return;

    const int mouseX = event->pos().x();
    const double frac = qBound(0.0, static_cast<double>(mouseX - margin) / trackWidth, 1.0);
    const double currentSec = frac * m_duration;

    for (const auto &seg : m_segments) {
        if (currentSec >= seg.start && currentSec <= seg.end) {
            const QString tip = QStringLiteral("⚠️ [%1s - %2s]\n%3")
                                    .arg(seg.start, 0, 'f', 2)
                                    .arg(seg.end, 0, 'f', 2)
                                    .arg(seg.errorMsg);
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
            return;
        }
    }
}
