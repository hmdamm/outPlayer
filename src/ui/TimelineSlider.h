// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QSlider>
#include "engine/IPlaybackEngine.h"

namespace outplayer::ui {

class TimelineSlider : public QSlider {
    Q_OBJECT

public:
    explicit TimelineSlider(QWidget* parent = nullptr);
    ~TimelineSlider() override = default;

    void setDuration(double seconds);
    void setPosition(double seconds);
    void setBufferProgress(double fraction);
    void setChapters(const QList<engine::ChapterInfo>& chapters);

    [[nodiscard]] double currentSeconds() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] double bufferProgress() const noexcept;
    [[nodiscard]] const QList<engine::ChapterInfo>& chapters() const noexcept;
    [[nodiscard]] bool isScrubbing() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void seekRequested(double targetSeconds);
    void scrubStarted();
    void scrubMoved(double targetSeconds);
    void scrubEnded(double targetSeconds);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    [[nodiscard]] double secondsFromPosition(double x) const noexcept;
    void updateTooltip(const QPointF& pos);

    double m_duration{0.0};
    double m_currentSeconds{0.0};
    double m_bufferProgress{0.0};
    QList<engine::ChapterInfo> m_chapters;
    bool m_isScrubbing{false};
    bool m_isHovered{false};
    double m_hoverX{-1.0};
};

} // namespace outplayer::ui
