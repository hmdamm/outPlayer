// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QPushButton;

namespace outplayer::ui {

class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget* parent = nullptr);
    ~CustomTitleBar() override = default;

    CustomTitleBar(const CustomTitleBar&) = delete;
    CustomTitleBar& operator=(const CustomTitleBar&) = delete;
    CustomTitleBar(CustomTitleBar&&) = delete;
    CustomTitleBar& operator=(CustomTitleBar&&) = delete;

    void setTitle(const QString& title);
    void setMaximizedState(bool isMaximized);

    [[nodiscard]] QString title() const noexcept;
    [[nodiscard]] bool isMaximizedState() const noexcept;

    [[nodiscard]] QLabel* titleLabel() const noexcept;
    [[nodiscard]] QPushButton* minimizeButton() const noexcept;
    [[nodiscard]] QPushButton* maximizeButton() const noexcept;
    [[nodiscard]] QPushButton* closeButton() const noexcept;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void applyStyle();
    void updateTitleText();

    QLabel* m_iconLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_minBtn{nullptr};
    QPushButton* m_maxBtn{nullptr};
    QPushButton* m_closeBtn{nullptr};

    QString m_rawTitle{QStringLiteral("outPlayer")};
    bool m_isMaximized{false};
};

} // namespace outplayer::ui
