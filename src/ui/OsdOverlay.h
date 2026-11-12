// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>

namespace outplayer::ui {

enum class OsdPosition {
    TopLeft,
    Center,
    BottomLeft
};

class OsdOverlay : public QWidget {
    Q_OBJECT

public:
    explicit OsdOverlay(QWidget* parent = nullptr);
    ~OsdOverlay() override = default;

    void showMessage(const QString& text, int durationMs = 1500, OsdPosition pos = OsdPosition::TopLeft);
    void clear();

    [[nodiscard]] QString currentText() const noexcept;
    [[nodiscard]] bool isVisibleOsd() const noexcept;
    [[nodiscard]] OsdPosition position() const noexcept;
    [[nodiscard]] qreal opacity() const noexcept;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void onFadeStep();
    void startFade();

private:
    void updateParentTracking();

    QString m_text;
    OsdPosition m_position{OsdPosition::TopLeft};
    qreal m_opacity{0.0};
    int m_fadeDurationMs{300};

    QTimer m_holdTimer;
    QTimer m_fadeTimer;
    QElapsedTimer m_fadeElapsedTimer;
    QPointer<QWidget> m_observedParent;
};

} // namespace outplayer::ui
