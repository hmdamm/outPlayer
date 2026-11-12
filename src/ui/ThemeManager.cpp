// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/ThemeManager.h"

#include <QHash>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>

namespace outplayer::ui {

namespace {

const QHash<QString, QString>& iconTemplates()
{
    static const QHash<QString, QString> templates = {
        {QStringLiteral("play"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="6,4 20,12 6,20" fill="%1"/></svg>)")},
        {QStringLiteral("pause"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect x="6" y="4" width="4" height="16" fill="%1" rx="1"/><rect x="14" y="4" width="4" height="16" fill="%1" rx="1"/></svg>)")},
        {QStringLiteral("stop"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect x="5" y="5" width="14" height="14" fill="%1" rx="1"/></svg>)")},
        {QStringLiteral("prev"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="19,5 9,12 19,19" fill="%1"/><rect x="5" y="5" width="3" height="14" fill="%1" rx="0.5"/></svg>)")},
        {QStringLiteral("next"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="5,5 15,12 5,19" fill="%1"/><rect x="16" y="5" width="3" height="14" fill="%1" rx="0.5"/></svg>)")},
        {QStringLiteral("volume"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="4,9 8,9 13,5 13,19 8,15 4,15" fill="%1"/><path d="M16 8c1.5 1.5 1.5 6.5 0 8" fill="none" stroke="%1" stroke-width="2" stroke-linecap="round"/><path d="M19 5c3 3 3 11 0 14" fill="none" stroke="%1" stroke-width="2" stroke-linecap="round"/></svg>)")},
        {QStringLiteral("mute"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="4,9 8,9 13,5 13,19 8,15 4,15" fill="%1"/><line x1="16" y1="9" x2="22" y2="15" stroke="%1" stroke-width="2" stroke-linecap="round"/><line x1="22" y1="9" x2="16" y2="15" stroke="%1" stroke-width="2" stroke-linecap="round"/></svg>)")},
        {QStringLiteral("settings"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M19.4 13c.04-.32.06-.66.06-1s-.02-.68-.06-1l2.11-1.65c.19-.15.24-.42.12-.64l-2-3.46c-.12-.22-.39-.3-.61-.22l-2.49 1c-.52-.4-1.08-.73-1.69-.98l-.38-2.65C14.46 2.18 14.25 2 14 2h-4c-.25 0-.46.18-.49.42l-.38 2.65c-.61.25-1.17.59-1.69.98l-2.49-1c-.23-.09-.49 0-.61.22l-2 3.46c-.13.22-.07.49.12.64L4.57 11c-.04.32-.07.65-.07 1s.02.68.06 1l-2.11 1.65c-.19.45-.25.42-.12.64l2 3.46c.12.22.39.3.61.22l2.49-1c.52.4 1.08.73 1.69.98l.38 2.65c.03.24.24.42.49.42h4c.25 0 .46-.18.49-.42l.38-2.65c.61-.25 1.17-.59 1.69-.98l2.49 1c.23.09.49 0 .61-.22l2-3.46c.12-.22.07-.49-.12-.64L19.4 13zM12 15.5c-1.93 0-3.5-1.57-3.5-3.5s1.57-3.5 3.5-3.5 3.5 1.57 3.5 3.5-1.57 3.5-3.5 3.5z" fill="%1"/></svg>)")},
        {QStringLiteral("playlist"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><line x1="4" y1="6" x2="20" y2="6" stroke="%1" stroke-width="2" stroke-linecap="round"/><line x1="4" y1="12" x2="20" y2="12" stroke="%1" stroke-width="2" stroke-linecap="round"/><line x1="4" y1="18" x2="14" y2="18" stroke="%1" stroke-width="2" stroke-linecap="round"/><polygon points="17,15 22,18 17,21" fill="%1"/></svg>)")},
        {QStringLiteral("control_panel"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><line x1="4" y1="6" x2="20" y2="6" stroke="%1" stroke-width="2" stroke-linecap="round"/><circle cx="8" cy="6" r="2.5" fill="%1"/><line x1="4" y1="12" x2="20" y2="12" stroke="%1" stroke-width="2" stroke-linecap="round"/><circle cx="16" cy="12" r="2.5" fill="%1"/><line x1="4" y1="18" x2="20" y2="18" stroke="%1" stroke-width="2" stroke-linecap="round"/><circle cx="10" cy="18" r="2.5" fill="%1"/></svg>)")},
        {QStringLiteral("capture"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M4 7h3l2-3h6l2 3h3a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V9a2 2 0 0 1 2-2z" fill="none" stroke="%1" stroke-width="2" stroke-linejoin="round"/><circle cx="12" cy="13" r="4" stroke="%1" stroke-width="2" fill="none"/></svg>)")},
        {QStringLiteral("bookmark"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><polygon points="6,3 18,3 18,21 12,17 6,21" fill="%1"/></svg>)")},
        {QStringLiteral("close"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><line x1="6" y1="6" x2="18" y2="18" stroke="%1" stroke-width="2" stroke-linecap="round"/><line x1="6" y1="18" x2="18" y2="6" stroke="%1" stroke-width="2" stroke-linecap="round"/></svg>)")},
        {QStringLiteral("minimize"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><line x1="5" y1="12" x2="19" y2="12" stroke="%1" stroke-width="2" stroke-linecap="round"/></svg>)")},
        {QStringLiteral("maximize"), QStringLiteral(
            R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect x="5" y="5" width="14" height="14" fill="none" stroke="%1" stroke-width="2" rx="1"/></svg>)")}
    };
    return templates;
}

} // namespace

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

void ThemeManager::setAccentColor(AccentColor color)
{
    if (m_accentColor == color) {
        return;
    }
    m_accentColor = color;
    emit themeChanged();
}

ThemeManager::AccentColor ThemeManager::accentColor() const
{
    return m_accentColor;
}

QString ThemeManager::accentHex() const
{
    switch (m_accentColor) {
    case AccentColor::Amber:
        return QStringLiteral("#ffaa00");
    case AccentColor::Blue:
        return QStringLiteral("#00a2ff");
    case AccentColor::Green:
        return QStringLiteral("#00cc66");
    case AccentColor::Crimson:
        return QStringLiteral("#ff3366");
    }
    return QStringLiteral("#ffaa00");
}

QString ThemeManager::styleSheet() const
{
    const QString accent = accentHex();
    return QString::fromUtf8(
R"(/* Outplayer Dark Theme */
QWidget {
    background-color: #1b1b1b;
    color: #e0e0e0;
    selection-background-color: %1;
    selection-color: #1b1b1b;
}

QMainWindow, QDialog {
    background-color: #1b1b1b;
}

QMenuBar {
    background-color: #1b1b1b;
    color: #e0e0e0;
    border-bottom: 1px solid #2e2e2e;
}

QMenuBar::item {
    background: transparent;
    padding: 4px 8px;
}

QMenuBar::item:selected {
    background-color: #383838;
    border-radius: 3px;
}

QMenu {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    padding: 4px 0px;
}

QMenu::item {
    padding: 6px 24px 6px 20px;
}

QMenu::item:selected {
    background-color: #383838;
    color: %1;
}

QMenu::separator {
    height: 1px;
    background-color: #2e2e2e;
    margin: 4px 8px;
}

QPushButton {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    border-radius: 4px;
    padding: 5px 12px;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #383838;
    border-color: %1;
}

QPushButton:pressed {
    background-color: #1b1b1b;
}

QPushButton:focus {
    border: 1px solid %1;
}

QLineEdit, QTextEdit, QPlainTextEdit {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    border-radius: 4px;
    padding: 4px 8px;
}

QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border: 1px solid %1;
}

QScrollBar:vertical {
    background-color: #1b1b1b;
    width: 10px;
    margin: 0px;
}

QScrollBar::handle:vertical {
    background-color: #2e2e2e;
    min-height: 20px;
    border-radius: 5px;
}

QScrollBar::handle:vertical:hover {
    background-color: #383838;
}

QScrollBar:horizontal {
    background-color: #1b1b1b;
    height: 10px;
    margin: 0px;
}

QScrollBar::handle:horizontal {
    background-color: #2e2e2e;
    min-width: 20px;
    border-radius: 5px;
}

QScrollBar::handle:horizontal:hover {
    background-color: #383838;
}

QScrollBar::add-line, QScrollBar::sub-line {
    width: 0px;
    height: 0px;
}

QSlider::groove:horizontal {
    height: 4px;
    background: #2e2e2e;
    border-radius: 2px;
}

QSlider::sub-page:horizontal {
    background: %1;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: %1;
    width: 12px;
    margin-top: -4px;
    margin-bottom: -4px;
    border-radius: 6px;
}

QSlider::handle:horizontal:hover {
    background: #e0e0e0;
}

QTabWidget::pane {
    border: 1px solid #2e2e2e;
    background-color: #252525;
}

QTabBar::tab {
    background-color: #1b1b1b;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    padding: 6px 14px;
}

QTabBar::tab:selected {
    background-color: #252525;
    border-bottom: 2px solid %1;
}

QTabBar::tab:hover:!selected {
    background-color: #383838;
}

QDockWidget {
    color: #e0e0e0;
}

QDockWidget::title {
    background-color: #252525;
    border: 1px solid #2e2e2e;
    padding: 6px;
    text-align: left;
}

QListView, QTreeView, QTableView {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    selection-background-color: #383838;
    selection-color: %1;
}

QToolTip {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #2e2e2e;
    padding: 4px;
}
)").arg(accent);
}

QPixmap ThemeManager::renderSvg(const QString& svgXml, const QSize& size) const
{
    if (svgXml.isEmpty() || size.width() <= 0 || size.height() <= 0) {
        return QPixmap();
    }

    QByteArray data = svgXml.toUtf8();
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return QPixmap();
    }

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(0, 0, size.width(), size.height()));
    return pixmap;
}

QIcon ThemeManager::icon(const QString& name, const QColor& color) const
{
    const auto& map = iconTemplates();
    auto it = map.constFind(name);
    if (it == map.constEnd()) {
        return QIcon();
    }

    const QString hexColor = color.isValid() ? color.name(QColor::HexRgb) : QStringLiteral("#e0e0e0");
    const QString svgXml = it.value().arg(hexColor);

    QIcon ic;
    ic.addPixmap(renderSvg(svgXml, QSize(16, 16)));
    ic.addPixmap(renderSvg(svgXml, QSize(24, 24)));
    ic.addPixmap(renderSvg(svgXml, QSize(32, 32)));
    ic.addPixmap(renderSvg(svgXml, QSize(48, 48)));
    return ic;
}

} // namespace outplayer::ui
