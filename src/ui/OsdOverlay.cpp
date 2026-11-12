// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/OsdOverlay.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

#include <algorithm>

namespace outplayer::ui {

OsdOverlay::OsdOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_holdTimer.setSingleShot(true);
    connect(&m_holdTimer, &QTimer::timeout, this, &OsdOverlay::startFade);
    connect(&m_fadeTimer, &QTimer::timeout, this, &OsdOverlay::onFadeStep);

    updateParentTracking();
}

void OsdOverlay::updateParentTracking() {
    QWidget* currentParent = parentWidget();
    if (m_observedParent && m_observedParent != currentParent) {
        m_observedParent->removeEventFilter(this);
    }
    m_observedParent = currentParent;
    if (m_observedParent) {
        m_observedParent->installEventFilter(this);
        setGeometry(m_observedParent->rect());
    }
}

bool OsdOverlay::event(QEvent* ev) {
    if (ev->type() == QEvent::ParentChange) {
        updateParentTracking();
    }
    return QWidget::event(ev);
}

bool OsdOverlay::eventFilter(QObject* watched, QEvent* ev) {
    if (watched == parentWidget()) {
        if (ev->type() == QEvent::Resize) {
            auto* resizeEvent = static_cast<QResizeEvent*>(ev);
            resize(resizeEvent->size());
        } else if (ev->type() == QEvent::Show) {
            setGeometry(parentWidget()->rect());
        }
    }
    return QWidget::eventFilter(watched, ev);
}

void OsdOverlay::showMessage(const QString& text, int durationMs, OsdPosition pos) {
    if (text.isEmpty() || durationMs <= 0) {
        clear();
        return;
    }

    m_holdTimer.stop();
    m_fadeTimer.stop();

    m_text = text;
    m_position = pos;
    m_opacity = 1.0;

    m_fadeDurationMs = std::min(300, durationMs);
    const int holdDurationMs = durationMs - m_fadeDurationMs;

    if (holdDurationMs > 0) {
        m_holdTimer.start(holdDurationMs);
    } else {
        startFade();
    }

    show();
    raise();
    update();
}

void OsdOverlay::clear() {
    m_holdTimer.stop();
    m_fadeTimer.stop();
    m_text.clear();
    m_opacity = 0.0;
    update();
}

void OsdOverlay::startFade() {
    m_holdTimer.stop();
    if (m_fadeDurationMs <= 0) {
        clear();
        return;
    }
    m_fadeElapsedTimer.restart();
    m_fadeTimer.start(16);
}

void OsdOverlay::onFadeStep() {
    if (m_fadeDurationMs <= 0) {
        clear();
        return;
    }

    const qint64 elapsed = m_fadeElapsedTimer.elapsed();
    if (elapsed >= m_fadeDurationMs) {
        clear();
    } else {
        m_opacity = 1.0 - (static_cast<qreal>(elapsed) / static_cast<qreal>(m_fadeDurationMs));
        m_opacity = std::clamp(m_opacity, 0.0, 1.0);
        update();
    }
}

QString OsdOverlay::currentText() const noexcept {
    return m_text;
}

bool OsdOverlay::isVisibleOsd() const noexcept {
    return !m_text.isEmpty() && m_opacity > 0.0;
}

OsdPosition OsdOverlay::position() const noexcept {
    return m_position;
}

qreal OsdOverlay::opacity() const noexcept {
    return m_opacity;
}

void OsdOverlay::paintEvent(QPaintEvent* /*event*/) {
    if (m_text.isEmpty() || m_opacity <= 0.0) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font(QStringLiteral("Inter"), 18, QFont::DemiBold);
    font.setStyleHint(QFont::SansSerif);
    painter.setFont(font);
    QFontMetrics fm(font);

    const QStringList lines = m_text.split(QLatin1Char('\n'));
    if (lines.isEmpty()) {
        return;
    }

    const int lineHeight = fm.lineSpacing();
    const int totalTextHeight = static_cast<int>(lines.size()) * lineHeight;

    int startY = 24 + fm.ascent();
    switch (m_position) {
    case OsdPosition::TopLeft:
        startY = 24 + fm.ascent();
        break;
    case OsdPosition::Center:
        startY = (height() - totalTextHeight + fm.ascent() - fm.descent()) / 2;
        if (startY < fm.ascent()) {
            startY = fm.ascent();
        }
        break;
    case OsdPosition::BottomLeft:
        // Position above bottom control bar (40px bar + 24px margin = 64px)
        startY = height() - 64 - totalTextHeight + fm.ascent();
        if (startY < 24 + fm.ascent()) {
            startY = 24 + fm.ascent();
        }
        break;
    }

    QPainterPath textPath;
    QPainterPath shadowPath;

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        int x = 24;
        if (m_position == OsdPosition::Center) {
            const int textWidth = fm.horizontalAdvance(line);
            x = (width() - textWidth) / 2;
        }
        const int y = startY + i * lineHeight;

        textPath.addText(x, y, font, line);
        shadowPath.addText(x + 1.0, y + 2.0, font, line);
    }

    painter.setOpacity(m_opacity);

    // 1. Ambient drop shadow (offset 1px 2px, alpha 0.85 = ~216)
    QPen shadowPen(QColor(0, 0, 0, 216));
    shadowPen.setWidthF(4.0);
    shadowPen.setJoinStyle(Qt::RoundJoin);
    shadowPen.setCapStyle(Qt::RoundCap);
    painter.strokePath(shadowPath, shadowPen);
    painter.fillPath(shadowPath, QColor(0, 0, 0, 216));

    // 2. 2px solid #000000 outline around text (stroke width 4.0 centered gives 2px outer outline)
    QPen outlinePen(QColor(0, 0, 0, 255));
    outlinePen.setWidthF(4.0);
    outlinePen.setJoinStyle(Qt::RoundJoin);
    outlinePen.setCapStyle(Qt::RoundCap);
    painter.strokePath(textPath, outlinePen);

    // 3. Crisp white #ffffff text fill
    painter.fillPath(textPath, QBrush(QColor(255, 255, 255, 255)));
}

} // namespace outplayer::ui
