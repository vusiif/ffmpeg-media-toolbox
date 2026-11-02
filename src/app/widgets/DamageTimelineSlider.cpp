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

void DamageTimelineSlider::setKeyframes(const QVector<double> &keyframes)
{
    m_keyframes = keyframes;
    update();
}

QVector<double> DamageTimelineSlider::keyframes() const
{
    return m_keyframes;
}

void DamageTimelineSlider::clearKeyframes()
{
    m_keyframes.clear();
    update();
}

void DamageTimelineSlider::setCutSegments(const QVector<QPair<double, double>> &segments)
{
    m_cutSegments = segments;
    update();
}

QVector<QPair<double, double>> DamageTimelineSlider::cutSegments() const
{
    return m_cutSegments;
}

void DamageTimelineSlider::clearCutSegments()
{
    m_cutSegments.clear();
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

    // 1. Paint All Saved Cut Segments (if any)
    if (!m_cutSegments.isEmpty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 153, 255, 70));
        for (const auto &pair : m_cutSegments) {
            if (pair.second > pair.first) {
                const double inFrac = qBound(0.0, pair.first / m_duration, 1.0);
                const double outFrac = qBound(0.0, pair.second / m_duration, 1.0);
                const int x1 = margin + static_cast<int>(inFrac * trackWidth);
                const int x2 = margin + static_cast<int>(outFrac * trackWidth);
                const int w = qMax(2, x2 - x1);
                painter.drawRoundedRect(QRect(x1, barY - 1, w, barHeight + 2), 2, 2);
            }
        }
    }

    // 2. Paint Current Active Trim Selected Range (if any)
    if (m_markOut > m_markIn) {
        const double inFrac = qBound(0.0, m_markIn / m_duration, 1.0);
        const double outFrac = qBound(0.0, m_markOut / m_duration, 1.0);

        const int x1 = margin + static_cast<int>(inFrac * trackWidth);
        const int x2 = margin + static_cast<int>(outFrac * trackWidth);
        const int w = qMax(2, x2 - x1);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 120, 212, 130));
        painter.drawRoundedRect(QRect(x1, barY - 1, w, barHeight + 2), 3, 3);

        painter.setPen(QPen(QColor(0, 120, 212, 240), 2));
        painter.drawLine(x1, barY - 3, x1, barY + barHeight + 3);
        painter.drawLine(x2, barY - 3, x2, barY + barHeight + 3);
    }

    // 3. Paint Keyframe Indicators (Ticks)
    if (!m_keyframes.isEmpty()) {
        painter.setPen(QPen(QColor(255, 185, 0, 200), 1));
        const int tickTop = barY - 4;
        const int tickBottom = barY;
        for (double kf : m_keyframes) {
            if (kf >= 0.0 && kf <= m_duration) {
                const double frac = kf / m_duration;
                const int x = margin + static_cast<int>(frac * trackWidth);
                painter.drawLine(x, tickTop, x, tickBottom);
            }
        }
    }

    // 4. Paint QC / Corrupted Segments (if any)
    if (!m_segments.isEmpty()) {
        for (const auto &seg : m_segments) {
            const double startFrac = qBound(0.0, seg.start / m_duration, 1.0);
            const double endFrac = qBound(0.0, seg.end / m_duration, 1.0);

            const int x1 = margin + static_cast<int>(startFrac * trackWidth);
            const int x2 = margin + static_cast<int>(endFrac * trackWidth);
            const int w = qMax(4, x2 - x1);

            const QColor col = seg.color();
            painter.setPen(QPen(col.darker(115), 1));
            painter.setBrush(col);
            painter.drawRoundedRect(QRect(x1, barY, w, barHeight), 2, 2);
        }
    }
}

void DamageTimelineSlider::mouseMoveEvent(QMouseEvent *event)
{
    ElaSlider::mouseMoveEvent(event);

    if (m_duration <= 0.0) {
        return;
    }

    const int margin = 10;
    const int trackWidth = width() - (margin * 2);
    if (trackWidth <= 0) return;

    const int mouseX = event->pos().x();
    const double frac = qBound(0.0, static_cast<double>(mouseX - margin) / trackWidth, 1.0);
    const double currentSec = frac * m_duration;

    // 1. Check QC segment tooltip
    if (!m_segments.isEmpty()) {
        for (const auto &seg : m_segments) {
            if (currentSec >= seg.start && currentSec <= seg.end) {
                const QString tip = QStringLiteral("⚠️ [%1] %2s - %3s (%4s)\n%5")
                                        .arg(seg.typeName())
                                        .arg(QString::number(seg.start, 'f', 2))
                                        .arg(QString::number(seg.end, 'f', 2))
                                        .arg(QString::number(seg.end - seg.start, 'f', 2))
                                        .arg(seg.errorMsg);
                QToolTip::showText(event->globalPosition().toPoint(), tip, this);
                return;
            }
        }
    }

    // 2. Check Keyframe tooltip
    if (!m_keyframes.isEmpty()) {
        for (double kf : m_keyframes) {
            if (std::abs(currentSec - kf) <= (m_duration / trackWidth * 3.0)) {
                const QString tip = QStringLiteral("🔑 Keyframe: %1s").arg(QString::number(kf, 'f', 3));
                QToolTip::showText(event->globalPosition().toPoint(), tip, this);
                return;
            }
        }
    }
}
