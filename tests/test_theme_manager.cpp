// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSignalSpy>
#include <QTest>

#include "ui/ThemeManager.h"

using namespace outplayer::ui;

class TestThemeManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void testSingleton();
    void testDefaultAccentColor();
    void testAccentColorSwitching();
    void testStyleSheet();
    void testRenderSvg();
    void testStandardIcons();
};

void TestThemeManager::init()
{
    // Reset to default Amber before each test
    ThemeManager::instance().setAccentColor(ThemeManager::AccentColor::Amber);
}

void TestThemeManager::testSingleton()
{
    ThemeManager& instance1 = ThemeManager::instance();
    ThemeManager& instance2 = ThemeManager::instance();
    QCOMPARE(&instance1, &instance2);
}

void TestThemeManager::testDefaultAccentColor()
{
    ThemeManager& theme = ThemeManager::instance();
    QCOMPARE(theme.accentColor(), ThemeManager::AccentColor::Amber);
    QCOMPARE(theme.accentHex(), QStringLiteral("#ffaa00"));
}

void TestThemeManager::testAccentColorSwitching()
{
    ThemeManager& theme = ThemeManager::instance();
    QSignalSpy spy(&theme, &ThemeManager::themeChanged);

    // Blue
    theme.setAccentColor(ThemeManager::AccentColor::Blue);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(theme.accentColor(), ThemeManager::AccentColor::Blue);
    QCOMPARE(theme.accentHex(), QStringLiteral("#00a2ff"));

    // Green
    theme.setAccentColor(ThemeManager::AccentColor::Green);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(theme.accentColor(), ThemeManager::AccentColor::Green);
    QCOMPARE(theme.accentHex(), QStringLiteral("#00cc66"));

    // Crimson
    theme.setAccentColor(ThemeManager::AccentColor::Crimson);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(theme.accentColor(), ThemeManager::AccentColor::Crimson);
    QCOMPARE(theme.accentHex(), QStringLiteral("#ff3366"));

    // Setting same color should not emit signal
    theme.setAccentColor(ThemeManager::AccentColor::Crimson);
    QCOMPARE(spy.count(), 3);

    // Back to Amber
    theme.setAccentColor(ThemeManager::AccentColor::Amber);
    QCOMPARE(spy.count(), 4);
    QCOMPARE(theme.accentColor(), ThemeManager::AccentColor::Amber);
    QCOMPARE(theme.accentHex(), QStringLiteral("#ffaa00"));
}

void TestThemeManager::testStyleSheet()
{
    ThemeManager& theme = ThemeManager::instance();

    theme.setAccentColor(ThemeManager::AccentColor::Amber);
    QString sheet = theme.styleSheet();
    QVERIFY(!sheet.isEmpty());
    QVERIFY(sheet.contains(QStringLiteral("#1b1b1b")));
    QVERIFY(sheet.contains(QStringLiteral("#252525")));
    QVERIFY(sheet.contains(QStringLiteral("#2e2e2e")));
    QVERIFY(sheet.contains(QStringLiteral("#383838")));
    QVERIFY(sheet.contains(QStringLiteral("#e0e0e0")));
    QVERIFY(sheet.contains(QStringLiteral("#ffaa00")));

    theme.setAccentColor(ThemeManager::AccentColor::Blue);
    QString blueSheet = theme.styleSheet();
    QVERIFY(blueSheet.contains(QStringLiteral("#00a2ff")));
    QVERIFY(!blueSheet.contains(QStringLiteral("#ffaa00")));
}

void TestThemeManager::testRenderSvg()
{
    ThemeManager& theme = ThemeManager::instance();

    const QString validSvg = QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\"><circle cx=\"12\" cy=\"12\" r=\"10\" fill=\"#ffaa00\"/></svg>");

    QPixmap pix = theme.renderSvg(validSvg, QSize(24, 24));
    QVERIFY(!pix.isNull());
    QCOMPARE(pix.size(), QSize(24, 24));

    QPixmap invalidPix = theme.renderSvg(QStringLiteral("invalid xml"), QSize(24, 24));
    QVERIFY(invalidPix.isNull());
}

void TestThemeManager::testStandardIcons()
{
    ThemeManager& theme = ThemeManager::instance();

    const QStringList iconNames = {
        QStringLiteral("play"),
        QStringLiteral("pause"),
        QStringLiteral("stop"),
        QStringLiteral("prev"),
        QStringLiteral("next"),
        QStringLiteral("volume"),
        QStringLiteral("mute"),
        QStringLiteral("settings"),
        QStringLiteral("playlist"),
        QStringLiteral("control_panel"),
        QStringLiteral("capture"),
        QStringLiteral("bookmark"),
        QStringLiteral("close"),
        QStringLiteral("minimize"),
        QStringLiteral("maximize")
    };

    for (const QString& name : iconNames) {
        // Default color
        QIcon defIcon = theme.icon(name);
        QVERIFY2(!defIcon.isNull(), qPrintable(QString("Icon '%1' is null").arg(name)));
        QPixmap defPix = defIcon.pixmap(24, 24);
        QVERIFY2(!defPix.isNull(), qPrintable(QString("Icon pixmap '%1' is null").arg(name)));

        // Custom color
        QIcon customIcon = theme.icon(name, QColor("#00a2ff"));
        QVERIFY2(!customIcon.isNull(), qPrintable(QString("Custom color icon '%1' is null").arg(name)));
    }

    // Invalid icon name should return null icon
    QIcon unknownIcon = theme.icon(QStringLiteral("nonexistent_unknown_icon"));
    QVERIFY(unknownIcon.isNull());
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    TestThemeManager tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_theme_manager.moc"
