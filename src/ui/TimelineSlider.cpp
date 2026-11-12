// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/TimelineSlider.h"
#include "ui/TimeFormatter.h"
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace outplayer::ui {

TimelineSlider::TimelineSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent)
{
    setOrientation(Qt::Horizontal);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(14);
}

void TimelineSlider::setDuration(double seconds)
{
    m_duration = std::max(0.0, seconds);
    if (m_currentSeconds > m_duration) {
        m_currentSeconds = m_duration;
    }
    update();
}

void TimelineSlider::setPosition(double seconds)
{
    if (m_isScrubbing) {
        return;
    }
    const double maxSec = m_duration > 0.0 ? m_duration : 0.0;
    const double clamped = std::clamp(seconds, 0.0, maxSec);
    if (!qFuzzyCompare(m_currentSeconds, clamped)) {
        m_currentSeconds = clamped;
        update();
    }
}

void TimelineSlider::setBufferProgress(double fraction)
{
    m_bufferProgress = std::clamp(fraction, 0.0, 1.0);
    update();
}

void TimelineSlider::setChapters(const QList<engine::ChapterInfo>& chapters)
{
    m_chapters = chapters;
    update();
}

double TimelineSlider::currentSeconds() const noexcept
{
    return m_currentSeconds;
}

double TimelineSlider::durationSeconds() const noexcept
{
    return m_duration;
}

double TimelineSlider::bufferProgress() const noexcept
{
    return m_bufferProgress;
}

const QList<engine::ChapterInfo>& TimelineSlider::chapters() const noexcept
{
    return m_chapters;
}

bool TimelineSlider::isScrubbing() const noexcept
{
    return m_isScrubbing;
}

QSize TimelineSlider::sizeHint() const
{
    return QSize(200, 14);
}

QSize TimelineSlider::minimumSizeHint() const
{
    return QSize(50, 14);
}

double TimelineSlider::secondsFromPosition(double x) const noexcept
{
    if (width() <= 0 || m_duration <= 0.0) {
        return 0.0;
    }
    const double fraction = std::clamp(x / static_cast<double>(width()), 0.0, 1.0);
    return fraction * m_duration;
}

void TimelineSlider::updateTooltip(const QPointF& pos)
{
    if (m_duration <= 0.0) {
        QToolTip::hideText();
        return;
    }
    const double hoverSec = secondsFromPosition(pos.x());
    const QString tip = TimeFormatter::formatTime(hoverSec, m_duration >= 3600.0);
    const QPoint globalPos = mapToGlobal(QPoint(static_cast<int>(pos.x()), 0));
    QToolTip::showText(globalPos, tip, this);
}

void TimelineSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isScrubbing = true;
        const double target = secondsFromPosition(event->position().x());
        m_currentSeconds = target;
        emit scrubStarted();
        emit seekRequested(target);
        update();
        event->accept();
        return;
    }
    QSlider::mousePressEvent(event);
}

void TimelineSlider::mouseMoveEvent(QMouseEvent* event)
{
    m_hoverX = event->position().x();
    updateTooltip(event->position());

    if (m_isScrubbing && (event->buttons() & Qt::LeftButton)) {
        const double target = secondsFromPosition(event->position().x());
        m_currentSeconds = target;
        emit scrubMoved(target);
        emit seekRequested(target);
        update();
        event->accept();
        return;
    }

    update();
    QSlider::mouseMoveEvent(event);
}

void TimelineSlider::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isScrubbing) {
        const double target = secondsFromPosition(event->position().x());
        m_currentSeconds = target;
        m_isScrubbing = false;
        emit scrubEnded(target);
        update();
        event->accept();
        return;
    }
    QSlider::mouseReleaseEvent(event);
}

void TimelineSlider::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    update();
    QSlider::enterEvent(event);
}

void TimelineSlider::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    m_hoverX = -1.0;
    QToolTip::hideText();
    update();
    QSlider::leaveEvent(event);
}

void TimelineSlider::wheelEvent(QWheelEvent* event)
{
    // Ignore wheel events so they bubble up to MainWindow/InputManager for volume control
    event->ignore();
}

void TimelineSlider::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    constexpr int grooveH = 4;
    const int grooveY = (h - grooveH) / 2;

    // 1. Background Groove (#383838)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x38, 0x38, 0x38));
    painter.drawRoundedRect(QRectF(0, grooveY, w, grooveH), 2.0, 2.0);

    // 2. Buffer Bar (#555555)
    if (m_bufferProgress > 0.0) {
        const double bufW = w * std::clamp(m_bufferProgress, 0.0, 1.0);
        painter.setBrush(QColor(0x55, 0x55, 0x55));
        painter.drawRoundedRect(QRectF(0, grooveY, bufW, grooveH), 2.0, 2.0);
    }

    // 3. Playback Progress (#e09b1f default, #ffaa00 on hover/scrub)
    const double progressFrac = m_duration > 0.0 ? std::clamp(m_currentSeconds / m_duration, 0.0, 1.0) : 0.0;
    const double progressW = w * progressFrac;
    if (progressW > 0.0) {
        const QColor progColor = (m_isHovered || m_isScrubbing)
                                     ? QColor(0xff, 0xaa, 0x00)
                                     : QColor(0xe0, 0x9b, 0x1f);
        painter.setBrush(progColor);
        painter.drawRoundedRect(QRectF(0, grooveY, progressW, grooveH), 2.0, 2.0);
    }

    // 4. Chapter Marker Ticks
    if (m_duration > 0.0 && !m_chapters.isEmpty()) {
        painter.setPen(QPen(QColor(255, 255, 255, 200), 1.5));
        for (const auto& ch : m_chapters) {
            if (ch.time >= 0.0 && ch.time <= m_duration) {
                const double x = (ch.time / m_duration) * w;
                painter.drawLine(QPointF(x, grooveY - 1), QPointF(x, grooveY + grooveH + 1));
            }
        }
    }

    // 5. Scrubber Thumb (10x10 idle, 14x14 with outer glow on hover or active scrub)
    if (m_duration > 0.0) {
        const double centerY = h / 2.0;
        if (m_isHovered || m_isScrubbing) {
            const double radius = 6.0;
            const double thumbX = std::clamp(progressW, radius, std::max(radius, w - radius));
            painter.setPen(QPen(QColor(0xff, 0xaa, 0x00), 2.0));
            painter.setBrush(QColor(0xff, 0xff, 0xff));
            painter.drawEllipse(QPointF(thumbX, centerY), radius, radius);
        } else {
            const double radius = 4.5;
            const double thumbX = std::clamp(progressW, radius, std::max(radius, w - radius));
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0xff, 0xff, 0xff));
            painter.drawEllipse(QPointF(thumbX, centerY), radius, radius);
        }
    }
}

} // namespace outplayer::ui
