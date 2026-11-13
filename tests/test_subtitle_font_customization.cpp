// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>

#include "core/SettingsManager.h"
#include "engine/MpvEngine.h"
#include "ui/ControlPanel.h"
#include "ui/PreferencesDialog.h"

using namespace outplayer;

class TestSubtitleFontCustomization : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testEngineSubtitleAppearanceProperties();
    void testEngineSettingsPersistenceOnInitialize();
    void testPreferencesDialogSubtitlesPageControlsAndPreview();
    void testPreferencesDialogTransactionalRollbackAndCommit();
    void testPreferencesDialogResetDefaults();
    void testControlPanelSubtitlesSync();

private:
    QTemporaryDir m_tempDir;
};

void TestSubtitleFontCustomization::initTestCase()
{
    QVERIFY(qApp != nullptr);
    QVERIFY(m_tempDir.isValid());
    core::SettingsManager::instance().init(m_tempDir.filePath("test_settings.ini"));
}

void TestSubtitleFontCustomization::cleanup()
{
    core::SettingsManager::instance().resetToDefaults();
}

void TestSubtitleFontCustomization::testEngineSubtitleAppearanceProperties()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    // Verify initial default values
    QCOMPARE(engine.subtitleFont(), QStringLiteral("sans-serif"));
    QCOMPARE(engine.subtitleFontSize(), 24);
    QCOMPARE(engine.subtitleColor(), QColor(255, 255, 255, 255));
    QCOMPARE(engine.isSubtitleBold(), false);
    QCOMPARE(engine.isSubtitleItalic(), false);
    QCOMPARE(engine.subtitleBorderColor(), QColor(0, 0, 0, 255));
    QCOMPARE(engine.subtitleBorderSize(), 2.0);
    QCOMPARE(engine.subtitleShadowColor(), QColor(0, 0, 0, 128));
    QCOMPARE(engine.subtitleShadowOffset(), 0.0);

    QSignalSpy spyChanged(&engine, &engine::IPlaybackEngine::subtitleSettingsChanged);

    // Test Font Name
    engine.setSubtitleFont(QStringLiteral("DejaVu Sans"));
    QCOMPARE(engine.subtitleFont(), QStringLiteral("DejaVu Sans"));
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-font")), QStringLiteral("DejaVu Sans"));
    QVERIFY(spyChanged.count() >= 1);

    // Test Font Size
    spyChanged.clear();
    engine.setSubtitleFontSize(36);
    QCOMPARE(engine.subtitleFontSize(), 36);
    QCOMPARE(qRound(engine.getPropertyString(QStringLiteral("sub-font-size")).toDouble()), 36);
    QVERIFY(spyChanged.count() >= 1);

    // Test Font Bold
    spyChanged.clear();
    engine.setSubtitleBold(true);
    QCOMPARE(engine.isSubtitleBold(), true);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-bold")), QStringLiteral("yes"));
    QVERIFY(spyChanged.count() >= 1);

    engine.setSubtitleBold(false);
    QCOMPARE(engine.isSubtitleBold(), false);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-bold")), QStringLiteral("no"));

    // Test Font Italic
    spyChanged.clear();
    engine.setSubtitleItalic(true);
    QCOMPARE(engine.isSubtitleItalic(), true);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-italic")), QStringLiteral("yes"));
    QVERIFY(spyChanged.count() >= 1);

    engine.setSubtitleItalic(false);
    QCOMPARE(engine.isSubtitleItalic(), false);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-italic")), QStringLiteral("no"));

    // Test Subtitle Color
    spyChanged.clear();
    const QColor yellowColor(255, 255, 0, 255);
    engine.setSubtitleColor(yellowColor);
    QCOMPARE(engine.subtitleColor(), yellowColor);
    QVERIFY(spyChanged.count() >= 1);
    // mpv property sub-color check
    const QString subColorProp = engine.getPropertyString(QStringLiteral("sub-color"));
    QVERIFY(!subColorProp.isEmpty());

    // Test Border Color & Size
    spyChanged.clear();
    const QColor blueColor(0, 0, 255, 255);
    engine.setSubtitleBorderColor(blueColor);
    QCOMPARE(engine.subtitleBorderColor(), blueColor);
    QVERIFY(spyChanged.count() >= 1);

    spyChanged.clear();
    engine.setSubtitleBorderSize(4.0);
    QCOMPARE(engine.subtitleBorderSize(), 4.0);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-border-size")).toDouble(), 4.0);
    QVERIFY(spyChanged.count() >= 1);

    // Test Shadow Color & Offset
    spyChanged.clear();
    const QColor shadowCol(0, 0, 0, 200);
    engine.setSubtitleShadowColor(shadowCol);
    QCOMPARE(engine.subtitleShadowColor(), shadowCol);
    QVERIFY(spyChanged.count() >= 1);

    spyChanged.clear();
    engine.setSubtitleShadowOffset(3.0);
    QCOMPARE(engine.subtitleShadowOffset(), 3.0);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-shadow-offset")).toDouble(), 3.0);
    QVERIFY(spyChanged.count() >= 1);
}

void TestSubtitleFontCustomization::testEngineSettingsPersistenceOnInitialize()
{
    // Write custom subtitle settings to SettingsManager
    auto& settings = core::SettingsManager::instance();
    settings.set(QStringLiteral("Subtitles/Font"), QStringLiteral("Liberation Sans"));
    settings.set(QStringLiteral("Subtitles/FontSize"), 40);
    settings.set(QStringLiteral("Subtitles/Color"), QStringLiteral("#ffff00"));
    settings.set(QStringLiteral("Subtitles/Bold"), true);
    settings.set(QStringLiteral("Subtitles/Italic"), true);
    settings.set(QStringLiteral("Subtitles/BorderColor"), QStringLiteral("#0000ff"));
    settings.set(QStringLiteral("Subtitles/BorderSize"), 3.5);
    settings.set(QStringLiteral("Subtitles/ShadowColor"), QStringLiteral("#80000000"));
    settings.set(QStringLiteral("Subtitles/ShadowOffset"), 2.5);
    settings.sync();

    // Now instantiate and initialize a fresh MpvEngine
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    // Verify properties were loaded from SettingsManager
    QCOMPARE(engine.subtitleFont(), QStringLiteral("Liberation Sans"));
    QCOMPARE(engine.subtitleFontSize(), 40);
    QCOMPARE(engine.subtitleColor(), QColor(QStringLiteral("#ffff00")));
    QCOMPARE(engine.isSubtitleBold(), true);
    QCOMPARE(engine.isSubtitleItalic(), true);
    QCOMPARE(engine.subtitleBorderColor(), QColor(QStringLiteral("#0000ff")));
    QCOMPARE(engine.subtitleBorderSize(), 3.5);
    QCOMPARE(engine.subtitleShadowColor(), QColor(QStringLiteral("#80000000")));
    QCOMPARE(engine.subtitleShadowOffset(), 2.5);

    // Verify mpv properties received the loaded settings
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-font")), QStringLiteral("Liberation Sans"));
    QCOMPARE(qRound(engine.getPropertyString(QStringLiteral("sub-font-size")).toDouble()), 40);
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-bold")), QStringLiteral("yes"));
    QCOMPARE(engine.getPropertyString(QStringLiteral("sub-italic")), QStringLiteral("yes"));
}

void TestSubtitleFontCustomization::testPreferencesDialogSubtitlesPageControlsAndPreview()
{
    auto& settings = core::SettingsManager::instance();
    settings.resetToDefaults();

    ui::PreferencesDialog dialog(&settings);

    // Check all controls exist
    QVERIFY(dialog.subtitleFontCombo() != nullptr);
    QVERIFY(dialog.subtitleFontSizeSpinBox() != nullptr);
    QCOMPARE(dialog.subtitleFontSizeSpinBox()->minimum(), 10);
    QCOMPARE(dialog.subtitleFontSizeSpinBox()->maximum(), 96);
    QVERIFY(dialog.subtitleBoldCheckBox() != nullptr);
    QVERIFY(dialog.subtitleItalicCheckBox() != nullptr);
    QVERIFY(dialog.subtitleColorButton() != nullptr);
    QVERIFY(dialog.subtitleBorderColorButton() != nullptr);
    QVERIFY(dialog.subtitleBorderSizeSpinBox() != nullptr);
    QCOMPARE(dialog.subtitleBorderSizeSpinBox()->minimum(), 0.0);
    QCOMPARE(dialog.subtitleBorderSizeSpinBox()->maximum(), 10.0);
    QVERIFY(dialog.subtitleShadowColorButton() != nullptr);
    QVERIFY(dialog.subtitleShadowOffsetSpinBox() != nullptr);
    QCOMPARE(dialog.subtitleShadowOffsetSpinBox()->minimum(), 0.0);
    QCOMPARE(dialog.subtitleShadowOffsetSpinBox()->maximum(), 10.0);
    QVERIFY(dialog.subtitlePreviewLabel() != nullptr);

    // Modify controls and check preview updates
    dialog.subtitleFontSizeSpinBox()->setValue(32);
    dialog.subtitleBoldCheckBox()->setChecked(true);
    dialog.subtitleItalicCheckBox()->setChecked(true);
    dialog.setSubtitleColor(QColor(Qt::yellow));
    dialog.setSubtitleBorderColor(QColor(Qt::blue));
    dialog.subtitleBorderSizeSpinBox()->setValue(3.0);
    dialog.setSubtitleShadowColor(QColor(0, 0, 0, 180));
    dialog.subtitleShadowOffsetSpinBox()->setValue(2.0);

    // Preview label text should have sample subtitle text
    QVERIFY(!dialog.subtitlePreviewLabel()->text().isEmpty());
    // Preview label styleSheet or font should reflect modifications
    const QString previewStyle = dialog.subtitlePreviewLabel()->styleSheet();
    QVERIFY(previewStyle.contains(QStringLiteral("color: #ffff00"), Qt::CaseInsensitive) ||
            previewStyle.contains(QStringLiteral("yellow"), Qt::CaseInsensitive) ||
            previewStyle.contains(QStringLiteral("font-weight: bold"), Qt::CaseInsensitive));
}

void TestSubtitleFontCustomization::testPreferencesDialogTransactionalRollbackAndCommit()
{
    auto& settings = core::SettingsManager::instance();
    settings.resetToDefaults();

    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    {
        // Test Rollback on Reject
        ui::PreferencesDialog dialog(&settings);
        dialog.setPlaybackEngine(&engine);

        dialog.subtitleFontSizeSpinBox()->setValue(44);
        dialog.subtitleBoldCheckBox()->setChecked(true);
        dialog.subtitleItalicCheckBox()->setChecked(true);
        dialog.setSubtitleColor(QColor(Qt::magenta));
        dialog.subtitleBorderSizeSpinBox()->setValue(5.0);

        // Cancel / reject
        dialog.reject();

        // Verify settings were NOT updated
        QCOMPARE(settings.get(QStringLiteral("Subtitles/FontSize"), 24).toInt(), 24);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/Bold"), false).toBool(), false);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/Italic"), false).toBool(), false);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/BorderSize"), 2.0).toDouble(), 2.0);

        // Verify engine was NOT changed to the cancelled values
        QCOMPARE(engine.subtitleFontSize(), 24);
        QCOMPARE(engine.isSubtitleBold(), false);
    }

    {
        // Test Commit on Apply
        ui::PreferencesDialog dialog(&settings);
        dialog.setPlaybackEngine(&engine);
        QSignalSpy spyApplied(&dialog, &ui::PreferencesDialog::settingsApplied);

        dialog.subtitleFontSizeSpinBox()->setValue(38);
        dialog.subtitleBoldCheckBox()->setChecked(true);
        dialog.subtitleItalicCheckBox()->setChecked(true);
        dialog.setSubtitleColor(QColor(Qt::cyan));
        dialog.setSubtitleBorderColor(QColor(Qt::red));
        dialog.subtitleBorderSizeSpinBox()->setValue(4.5);
        dialog.setSubtitleShadowColor(QColor(0, 0, 0, 150));
        dialog.subtitleShadowOffsetSpinBox()->setValue(2.5);

        dialog.applyButton()->click();
        QCOMPARE(spyApplied.count(), 1);

        // Verify persisted in SettingsManager
        QCOMPARE(settings.get(QStringLiteral("Subtitles/FontSize")).toInt(), 38);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/Bold")).toBool(), true);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/Italic")).toBool(), true);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/Color")).toString(), QColor(Qt::cyan).name(QColor::HexArgb));
        QCOMPARE(settings.get(QStringLiteral("Subtitles/BorderColor")).toString(), QColor(Qt::red).name(QColor::HexArgb));
        QCOMPARE(settings.get(QStringLiteral("Subtitles/BorderSize")).toDouble(), 4.5);
        QCOMPARE(settings.get(QStringLiteral("Subtitles/ShadowOffset")).toDouble(), 2.5);

        // Verify applied to engine
        QCOMPARE(engine.subtitleFontSize(), 38);
        QCOMPARE(engine.isSubtitleBold(), true);
        QCOMPARE(engine.isSubtitleItalic(), true);
        QCOMPARE(engine.subtitleColor(), QColor(Qt::cyan));
        QCOMPARE(engine.subtitleBorderColor(), QColor(Qt::red));
        QCOMPARE(engine.subtitleBorderSize(), 4.5);
        QCOMPARE(engine.subtitleShadowOffset(), 2.5);
    }
}

void TestSubtitleFontCustomization::testPreferencesDialogResetDefaults()
{
    auto& settings = core::SettingsManager::instance();
    ui::PreferencesDialog dialog(&settings);

    dialog.subtitleFontSizeSpinBox()->setValue(50);
    dialog.subtitleBoldCheckBox()->setChecked(true);
    dialog.subtitleItalicCheckBox()->setChecked(true);
    dialog.setSubtitleColor(QColor(Qt::red));
    dialog.subtitleBorderSizeSpinBox()->setValue(6.0);
    dialog.subtitleShadowOffsetSpinBox()->setValue(5.0);

    // Trigger reset defaults
    dialog.resetDefaults();

    QCOMPARE(dialog.subtitleFontSizeSpinBox()->value(), 24);
    QCOMPARE(dialog.subtitleBoldCheckBox()->isChecked(), false);
    QCOMPARE(dialog.subtitleItalicCheckBox()->isChecked(), false);
    QCOMPARE(dialog.subtitleColor(), QColor(255, 255, 255, 255));
    QCOMPARE(dialog.subtitleBorderColor(), QColor(0, 0, 0, 255));
    QCOMPARE(dialog.subtitleBorderSizeSpinBox()->value(), 2.0);
    QCOMPARE(dialog.subtitleShadowColor(), QColor(0, 0, 0, 128));
    QCOMPARE(dialog.subtitleShadowOffsetSpinBox()->value(), 0.0);
}

void TestSubtitleFontCustomization::testControlPanelSubtitlesSync()
{
    engine::MpvEngine engine;
    QVERIFY(engine.initialize());

    ui::ControlPanel panel(&engine);

    // Verify font family combo and font size selector exist in ControlPanel
    QVERIFY(panel.subtitleFontComboBox() != nullptr);
    QVERIFY(panel.subtitleFontSizeSpinBox() != nullptr);
    QCOMPARE(panel.subtitleFontSizeSpinBox()->minimum(), 10);
    QCOMPARE(panel.subtitleFontSizeSpinBox()->maximum(), 96);

    // Initial values should match engine
    QCOMPARE(panel.subtitleFontSizeSpinBox()->value(), engine.subtitleFontSize());

    // Changing font size in ControlPanel updates engine
    panel.subtitleFontSizeSpinBox()->setValue(34);
    QCOMPARE(engine.subtitleFontSize(), 34);

    // Changing font in ControlPanel updates engine
    const QString sampleFont = panel.subtitleFontComboBox()->currentFont().family();
    panel.subtitleFontComboBox()->setCurrentFont(QFont(sampleFont));
    QCOMPARE(engine.subtitleFont(), sampleFont);

    // Changing engine properties updates ControlPanel
    engine.setSubtitleFontSize(42);
    QCOMPARE(panel.subtitleFontSizeSpinBox()->value(), 42);
}

QTEST_MAIN(TestSubtitleFontCustomization)
#include "test_subtitle_font_customization.moc"
