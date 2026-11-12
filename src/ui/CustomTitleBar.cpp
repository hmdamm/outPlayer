// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/CustomTitleBar.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QWindow>

#include <algorithm>

namespace outplayer::ui {

CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(30);

    setupUi();
    applyStyle();
}

void CustomTitleBar::setupUi()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(16, 16);
    m_iconLabel->setText(QStringLiteral("▶"));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet(QStringLiteral("color: #ff9800; font-weight: bold; font-size: 11px;"));
    layout->addWidget(m_iconLabel);

    m_titleLabel = new QLabel(m_rawTitle, this);
    m_titleLabel->setStyleSheet(QStringLiteral("color: #dcdcdc; font-size: 12px; font-weight: 500;"));
    layout->addWidget(m_titleLabel, 1);

    m_minBtn = new QPushButton(QStringLiteral("−"), this);
    m_minBtn->setFixedSize(45, 30);
    m_minBtn->setToolTip(QStringLiteral("Minimize"));
    m_minBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_minBtn, &QPushButton::clicked, this, &CustomTitleBar::minimizeClicked);
    layout->addWidget(m_minBtn);

    m_maxBtn = new QPushButton(QStringLiteral("□"), this);
    m_maxBtn->setFixedSize(45, 30);
    m_maxBtn->setToolTip(QStringLiteral("Maximize"));
    m_maxBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_maxBtn, &QPushButton::clicked, this, &CustomTitleBar::maximizeRestoreClicked);
    layout->addWidget(m_maxBtn);

    m_closeBtn = new QPushButton(QStringLiteral("✕"), this);
    m_closeBtn->setObjectName(QStringLiteral("closeBtn"));
    m_closeBtn->setFixedSize(45, 30);
    m_closeBtn->setToolTip(QStringLiteral("Close"));
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_closeBtn, &QPushButton::clicked, this, &CustomTitleBar::closeClicked);
    layout->addWidget(m_closeBtn);
}

void CustomTitleBar::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "outplayer--ui--CustomTitleBar {"
        "    background-color: #1b1b1b;"
        "    border-bottom: 1px solid #242424;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #cccccc;"
        "    border: none;"
        "    font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2e2e2e;"
        "    color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #383838;"
        "}"
        "QPushButton#closeBtn:hover {"
        "    background-color: #c42b1c;"
        "    color: #ffffff;"
        "}"
        "QPushButton#closeBtn:pressed {"
        "    background-color: #9e2216;"
        "    color: #ffffff;"
        "}"
    ));
}

void CustomTitleBar::setTitle(const QString& title)
{
    m_rawTitle = title;
    updateTitleText();
}

void CustomTitleBar::setMaximizedState(bool isMaximized)
{
    m_isMaximized = isMaximized;
    if (m_maxBtn) {
        m_maxBtn->setText(isMaximized ? QStringLiteral("❐") : QStringLiteral("□"));
        m_maxBtn->setToolTip(isMaximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
    }
}

QString CustomTitleBar::title() const noexcept
{
    return m_rawTitle;
}

bool CustomTitleBar::isMaximizedState() const noexcept
{
    return m_isMaximized;
}

QLabel* CustomTitleBar::titleLabel() const noexcept
{
    return m_titleLabel;
}

QLabel* CustomTitleBar::iconLabel() const noexcept
{
    return m_iconLabel;
}

QPushButton* CustomTitleBar::minimizeButton() const noexcept
{
    return m_minBtn;
}

QPushButton* CustomTitleBar::maximizeButton() const noexcept
{
    return m_maxBtn;
}

QPushButton* CustomTitleBar::closeButton() const noexcept
{
    return m_closeBtn;
}

QSize CustomTitleBar::sizeHint() const
{
    return QSize(640, 30);
}

QSize CustomTitleBar::minimumSizeHint() const
{
    return QSize(200, 30);
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_iconLabel && m_iconLabel->geometry().contains(event->pos())) {
            emit logoClicked();
            event->accept();
            return;
        }
        if (window()) {
            if (!window()->windowHandle()) {
                window()->winId();
            }
            if (window()->windowHandle()) {
                window()->windowHandle()->startSystemMove();
            }
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreClicked();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CustomTitleBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateTitleText();
}

void CustomTitleBar::updateTitleText()
{
    if (!m_titleLabel) {
        return;
    }

    int availableWidth = m_titleLabel->width();
    if (availableWidth <= 0) {
        availableWidth = std::max(60, width() - 180);
    }

    QFontMetrics fm(m_titleLabel->font());
    m_titleLabel->setText(fm.elidedText(m_rawTitle, Qt::ElideRight, availableWidth));
}

} // namespace outplayer::ui
