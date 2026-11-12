// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

#include "core/SettingsManager.h"
#include "ui/PreferencesDialog.h"
#include "ui/ThemeManager.h"

using namespace outplayer;

class TestPreferencesDialog : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testDialogCreationAndGeometry();
    void testCategoryTreeStructure();
    void testCategorySelection();
    void testSearchFilter();
    void testTemporaryStateAndCancel();
    void testSaveAndApplySettings();
    void testResetDefaults();
    void testAssociationsPageActions();
    void testHotkeysPage();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    core::SettingsManager* m_settings{nullptr};
};

void TestPreferencesDialog::initTestCase()
{
    // Ensure Qt QApplication exists (handled by QTEST_MAIN)
}

void TestPreferencesDialog::init()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    QString iniPath = m_tempDir->filePath("test_preferences.ini");
    m_settings = &core::SettingsManager::instance();
    m_settings->init(iniPath);
    ui::ThemeManager::instance().setAccentColor(ui::ThemeManager::AccentColor::Amber);
}

void TestPreferencesDialog::cleanup()
{
    m_tempDir.reset();
}

void TestPreferencesDialog::testDialogCreationAndGeometry()
{
    ui::PreferencesDialog dialog(m_settings);
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Preferences"));
    QVERIFY(dialog.width() >= 640);
    QVERIFY(dialog.height() >= 480);
    QVERIFY(dialog.categoryTree() != nullptr);
    QVERIFY(dialog.stackedWidget() != nullptr);
    QVERIFY(dialog.searchEdit() != nullptr);
    QVERIFY(dialog.okButton() != nullptr);
    QVERIFY(dialog.cancelButton() != nullptr);
    QVERIFY(dialog.applyButton() != nullptr);
    QVERIFY(dialog.resetDefaultsButton() != nullptr);
}

void TestPreferencesDialog::testCategoryTreeStructure()
{
    ui::PreferencesDialog dialog(m_settings);
    QTreeWidget* tree = dialog.categoryTree();
    QVERIFY(tree != nullptr);

    // Categories: General, Playback, Video, Audio, Subtitles, Associations, Hotkeys
    QStringList expectedCategories = {
        QStringLiteral("General"),
        QStringLiteral("Playback"),
        QStringLiteral("Video"),
        QStringLiteral("Audio"),
        QStringLiteral("Subtitles"),
        QStringLiteral("Associations"),
        QStringLiteral("Hotkeys")
    };

    QCOMPARE(tree->topLevelItemCount(), expectedCategories.size());

    for (int i = 0; i < expectedCategories.size(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        QVERIFY(item != nullptr);
        QCOMPARE(item->text(0), expectedCategories[i]);
    }

    // Check subcategories for General
    QTreeWidgetItem* genItem = tree->topLevelItem(0);
    QStringList genSubs = {
        QStringLiteral("Startup"),
        QStringLiteral("Skin/Theme"),
        QStringLiteral("OSD"),
        QStringLiteral("Window")
    };
    QCOMPARE(genItem->childCount(), genSubs.size());
    for (int i = 0; i < genSubs.size(); ++i) {
        QCOMPARE(genItem->child(i)->text(0), genSubs[i]);
    }

    // Check subcategories for Playback
    QTreeWidgetItem* playItem = tree->topLevelItem(1);
    QStringList playSubs = {
        QStringLiteral("Speed"),
        QStringLiteral("Seeking/Jump"),
        QStringLiteral("Resume")
    };
    QCOMPARE(playItem->childCount(), playSubs.size());

    // Check subcategories for Video
    QTreeWidgetItem* vidItem = tree->topLevelItem(2);
    QStringList vidSubs = {
        QStringLiteral("Hardware Decoding"),
        QStringLiteral("Deinterlacing"),
        QStringLiteral("Aspect Ratio")
    };
    QCOMPARE(vidItem->childCount(), vidSubs.size());

    // Check subcategories for Audio
    QTreeWidgetItem* audItem = tree->topLevelItem(3);
    QStringList audSubs = {
        QStringLiteral("Output Device"),
        QStringLiteral("Normalizer"),
        QStringLiteral("Equalizer")
    };
    QCOMPARE(audItem->childCount(), audSubs.size());

    // Check subcategories for Subtitles
    QTreeWidgetItem* subItem = tree->topLevelItem(4);
    QStringList subSubs = {
        QStringLiteral("Language"),
        QStringLiteral("Font"),
        QStringLiteral("Size"),
        QStringLiteral("Position")
    };
    QCOMPARE(subItem->childCount(), subSubs.size());

    // Check subcategories for Associations
    QTreeWidgetItem* assocItem = tree->topLevelItem(5);
    QStringList assocSubs = {
        QStringLiteral("Video Formats"),
        QStringLiteral("Audio Formats"),
        QStringLiteral("Playlists")
    };
    QCOMPARE(assocItem->childCount(), assocSubs.size());

    // Check subcategories for Hotkeys
    QTreeWidgetItem* hkItem = tree->topLevelItem(6);
    QStringList hkSubs = {
        QStringLiteral("Shortcuts Table"),
        QStringLiteral("Rebind"),
        QStringLiteral("Reset Defaults")
    };
    QCOMPARE(hkItem->childCount(), hkSubs.size());
}

void TestPreferencesDialog::testCategorySelection()
{
    ui::PreferencesDialog dialog(m_settings);
    QStackedWidget* stack = dialog.stackedWidget();

    dialog.selectCategory(QStringLiteral("General"));
    QCOMPARE(stack->currentIndex(), 0);

    dialog.selectCategory(QStringLiteral("Playback"));
    QCOMPARE(stack->currentIndex(), 1);

    dialog.selectCategory(QStringLiteral("Video"));
    QCOMPARE(stack->currentIndex(), 2);

    dialog.selectCategory(QStringLiteral("Audio"));
    QCOMPARE(stack->currentIndex(), 3);

    dialog.selectCategory(QStringLiteral("Subtitles"));
    QCOMPARE(stack->currentIndex(), 4);

    dialog.selectCategory(QStringLiteral("Associations"));
    QCOMPARE(stack->currentIndex(), 5);

    dialog.selectCategory(QStringLiteral("Hotkeys"));
    QCOMPARE(stack->currentIndex(), 6);

    // Selecting a child category should map to its parent's page
    dialog.selectCategory(QStringLiteral("Hardware Decoding"));
    QCOMPARE(stack->currentIndex(), 2);

    dialog.selectCategory(QStringLiteral("Speed"));
    QCOMPARE(stack->currentIndex(), 1);
}

void TestPreferencesDialog::testSearchFilter()
{
    ui::PreferencesDialog dialog(m_settings);
    QTreeWidget* tree = dialog.categoryTree();

    // Search for "Hardware" -> Video item should be visible, General should be hidden
    dialog.setSearchFilter(QStringLiteral("Hardware"));

    QTreeWidgetItem* genItem = tree->topLevelItem(0);
    QTreeWidgetItem* vidItem = tree->topLevelItem(2);
    QVERIFY(genItem->isHidden());
    QVERIFY(!vidItem->isHidden());

    // Search for "Normalizer" -> Audio should be visible
    dialog.setSearchFilter(QStringLiteral("Normalizer"));
    QTreeWidgetItem* audItem = tree->topLevelItem(3);
    QVERIFY(!audItem->isHidden());
    QVERIFY(vidItem->isHidden());

    // Clear search -> all visible
    dialog.setSearchFilter(QStringLiteral(""));
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QVERIFY(!tree->topLevelItem(i)->isHidden());
    }
}

void TestPreferencesDialog::testTemporaryStateAndCancel()
{
    ui::PreferencesDialog dialog(m_settings);

    // Modify controls
    dialog.singleInstanceCheckBox()->setChecked(true);
    dialog.minimizeToTrayCheckBox()->setChecked(true);
    dialog.opacitySlider()->setValue(70);
    dialog.shortJumpSpinBox()->setValue(12);

    // In settings, values should NOT be updated yet
    QVERIFY(!m_settings->get(QStringLiteral("General/SingleInstance"), false).toBool());
    QVERIFY(!m_settings->get(QStringLiteral("General/MinimizeToTray"), false).toBool());
    QVERIFY(m_settings->get(QStringLiteral("General/WindowOpacity"), 100).toInt() != 70);

    // Cancel / reject
    dialog.reject();

    // Verify settings still untouched
    QVERIFY(!m_settings->get(QStringLiteral("General/SingleInstance"), false).toBool());
    QVERIFY(!m_settings->get(QStringLiteral("General/MinimizeToTray"), false).toBool());
}

void TestPreferencesDialog::testSaveAndApplySettings()
{
    ui::PreferencesDialog dialog(m_settings);
    QSignalSpy spy(&dialog, &ui::PreferencesDialog::settingsApplied);

    // Configure controls
    dialog.singleInstanceCheckBox()->setChecked(true);
    dialog.minimizeToTrayCheckBox()->setChecked(true);
    dialog.accentColorComboBox()->setCurrentText(QStringLiteral("Blue"));
    dialog.opacitySlider()->setValue(85);

    dialog.speedSpinBox()->setValue(1.25);
    dialog.shortJumpSpinBox()->setValue(10);
    dialog.mediumJumpSpinBox()->setValue(45);
    dialog.largeJumpSpinBox()->setValue(90);
    dialog.autoResumeCheckBox()->setChecked(false);

    dialog.hwdecComboBox()->setCurrentText(QStringLiteral("vaapi"));
    dialog.aspectRatioComboBox()->setCurrentText(QStringLiteral("16:9"));
    dialog.deinterlaceCheckBox()->setChecked(true);

    dialog.normalizerCheckBox()->setChecked(true);
    dialog.audioDelayStepSpinBox()->setValue(0.1);

    dialog.subtitleLanguageEdit()->setText(QStringLiteral("ja"));
    dialog.subtitleFontSizeSpinBox()->setValue(36);

    // Click Apply
    dialog.applyButton()->click();

    QCOMPARE(spy.count(), 1);

    // Check SettingsManager values
    QCOMPARE(m_settings->get(QStringLiteral("General/SingleInstance")).toBool(), true);
    QCOMPARE(m_settings->get(QStringLiteral("General/MinimizeToTray")).toBool(), true);
    QCOMPARE(m_settings->get(QStringLiteral("General/AccentColor")).toString(), QStringLiteral("Blue"));
    QCOMPARE(m_settings->get(QStringLiteral("General/WindowOpacity")).toInt(), 85);

    QCOMPARE(m_settings->get(QStringLiteral("Playback/DefaultSpeed")).toDouble(), 1.25);
    QCOMPARE(m_settings->get(QStringLiteral("Playback/ShortJumpStep")).toInt(), 10);
    QCOMPARE(m_settings->get(QStringLiteral("Playback/MediumJumpStep")).toInt(), 45);
    QCOMPARE(m_settings->get(QStringLiteral("Playback/LargeJumpStep")).toInt(), 90);
    QCOMPARE(m_settings->get(QStringLiteral("Playback/AutoResume")).toBool(), false);

    QCOMPARE(m_settings->get(QStringLiteral("Video/HardwareDecoder")).toString(), QStringLiteral("vaapi"));
    QCOMPARE(m_settings->get(QStringLiteral("Video/AspectRatio")).toString(), QStringLiteral("16:9"));
    QCOMPARE(m_settings->get(QStringLiteral("Video/Deinterlacing")).toBool(), true);

    QCOMPARE(m_settings->get(QStringLiteral("Audio/Normalizer")).toBool(), true);
    QCOMPARE(m_settings->get(QStringLiteral("Audio/AudioDelayStep")).toDouble(), 0.1);

    QCOMPARE(m_settings->get(QStringLiteral("Subtitles/DefaultLanguage")).toString(), QStringLiteral("ja"));
    QCOMPARE(m_settings->get(QStringLiteral("Subtitles/FontSize")).toInt(), 36);
}

void TestPreferencesDialog::testResetDefaults()
{
    ui::PreferencesDialog dialog(m_settings);

    // Change some values
    dialog.singleInstanceCheckBox()->setChecked(true);
    dialog.opacitySlider()->setValue(50);
    dialog.shortJumpSpinBox()->setValue(25);
    dialog.hwdecComboBox()->setCurrentText(QStringLiteral("nvdec"));
    dialog.subtitleFontSizeSpinBox()->setValue(48);

    // Reset defaults
    dialog.resetDefaults();

    // Verify UI controls restored to defaults
    QCOMPARE(dialog.singleInstanceCheckBox()->isChecked(), false);
    QCOMPARE(dialog.opacitySlider()->value(), 100);
    QCOMPARE(dialog.shortJumpSpinBox()->value(), 5);
    QCOMPARE(dialog.hwdecComboBox()->currentText(), QStringLiteral("auto"));
    QCOMPARE(dialog.subtitleFontSizeSpinBox()->value(), 24);
}

void TestPreferencesDialog::testAssociationsPageActions()
{
    ui::PreferencesDialog dialog(m_settings);
    auto boxes = dialog.extensionCheckBoxes();
    QVERIFY(!boxes.isEmpty());

    // Click Deselect All
    dialog.deselectAllExtensionsButton()->click();
    for (auto* cb : boxes) {
        QVERIFY(!cb->isChecked());
    }

    // Click Select All
    dialog.selectAllExtensionsButton()->click();
    for (auto* cb : boxes) {
        QVERIFY(cb->isChecked());
    }
}

void TestPreferencesDialog::testHotkeysPage()
{
    ui::PreferencesDialog dialog(m_settings);
    QTableWidget* table = dialog.hotkeysTable();
    QVERIFY(table != nullptr);
    QVERIFY(table->rowCount() > 0);

    // Filter hotkeys
    dialog.hotkeyFilterEdit()->setText(QStringLiteral("Play"));
    int visibleRows = 0;
    for (int r = 0; r < table->rowCount(); ++r) {
        if (!table->isRowHidden(r)) {
            visibleRows++;
        }
    }
    QVERIFY(visibleRows > 0);
    QVERIFY(visibleRows <= table->rowCount());

    // Reset filter
    dialog.hotkeyFilterEdit()->setText(QStringLiteral(""));
    int totalVisible = 0;
    for (int r = 0; r < table->rowCount(); ++r) {
        if (!table->isRowHidden(r)) {
            totalVisible++;
        }
    }
    QCOMPARE(totalVisible, table->rowCount());
}

QTEST_MAIN(TestPreferencesDialog)
#include "test_preferences_dialog.moc"
