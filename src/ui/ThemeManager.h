// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OUTPLAYER_UI_THEMEMANAGER_H
#define OUTPLAYER_UI_THEMEMANAGER_H

#include <QColor>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace outplayer::ui {

class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum class AccentColor {
        Amber,
        Blue,
        Green,
        Crimson
    };

    static ThemeManager& instance();

    void setAccentColor(AccentColor color);
    [[nodiscard]] AccentColor accentColor() const;
    [[nodiscard]] QString accentHex() const;

    [[nodiscard]] QString styleSheet() const;
    [[nodiscard]] QIcon icon(const QString& name, const QColor& color = QColor()) const;
    [[nodiscard]] QPixmap renderSvg(const QString& svgXml, const QSize& size) const;

signals:
    void themeChanged();

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    AccentColor m_accentColor{AccentColor::Amber};
};

} // namespace outplayer::ui

#endif // OUTPLAYER_UI_THEMEMANAGER_H
