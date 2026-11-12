// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OUTPLAYER_UI_PREFERENCESDIALOG_H
#define OUTPLAYER_UI_PREFERENCESDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace outplayer::core {
class SettingsManager;
}

namespace outplayer::ui {

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(core::SettingsManager* settings = nullptr, QWidget* parent = nullptr);
    ~PreferencesDialog() override = default;

    void loadSettings();
    void saveSettings();
    void resetDefaults();

    void selectCategory(const QString& categoryName);
    void setSearchFilter(const QString& query);

    // Dialog layout components
    [[nodiscard]] QTreeWidget* categoryTree() const noexcept { return m_categoryTree; }
    [[nodiscard]] QStackedWidget* stackedWidget() const noexcept { return m_stackedWidget; }
    [[nodiscard]] QLineEdit* searchEdit() const noexcept { return m_searchEdit; }
    [[nodiscard]] QPushButton* okButton() const noexcept { return m_okButton; }
    [[nodiscard]] QPushButton* cancelButton() const noexcept { return m_cancelButton; }
    [[nodiscard]] QPushButton* applyButton() const noexcept { return m_applyButton; }
    [[nodiscard]] QPushButton* resetDefaultsButton() const noexcept { return m_resetDefaultsButton; }

    // Page 0: General Controls
    [[nodiscard]] QCheckBox* singleInstanceCheckBox() const noexcept { return m_singleInstanceCheckBox; }
    [[nodiscard]] QCheckBox* minimizeToTrayCheckBox() const noexcept { return m_minimizeToTrayCheckBox; }
    [[nodiscard]] QComboBox* accentColorComboBox() const noexcept { return m_accentColorComboBox; }
    [[nodiscard]] QSlider* opacitySlider() const noexcept { return m_opacitySlider; }

    // Page 1: Playback Controls
    [[nodiscard]] QDoubleSpinBox* speedSpinBox() const noexcept { return m_speedSpinBox; }
    [[nodiscard]] QSpinBox* shortJumpSpinBox() const noexcept { return m_shortJumpSpinBox; }
    [[nodiscard]] QSpinBox* mediumJumpSpinBox() const noexcept { return m_mediumJumpSpinBox; }
    [[nodiscard]] QSpinBox* largeJumpSpinBox() const noexcept { return m_largeJumpSpinBox; }
    [[nodiscard]] QCheckBox* autoResumeCheckBox() const noexcept { return m_autoResumeCheckBox; }

    // Page 2: Video Controls
    [[nodiscard]] QComboBox* hwdecComboBox() const noexcept { return m_hwdecComboBox; }
    [[nodiscard]] QComboBox* aspectRatioComboBox() const noexcept { return m_aspectRatioComboBox; }
    [[nodiscard]] QCheckBox* deinterlaceCheckBox() const noexcept { return m_deinterlaceCheckBox; }

    // Page 3: Audio Controls
    [[nodiscard]] QComboBox* audioDeviceComboBox() const noexcept { return m_audioDeviceComboBox; }
    [[nodiscard]] QCheckBox* normalizerCheckBox() const noexcept { return m_normalizerCheckBox; }
    [[nodiscard]] QDoubleSpinBox* audioDelayStepSpinBox() const noexcept { return m_audioDelayStepSpinBox; }

    // Page 4: Subtitles Controls
    [[nodiscard]] QLineEdit* subtitleLanguageEdit() const noexcept { return m_subtitleLanguageEdit; }
    [[nodiscard]] QSpinBox* subtitleFontSizeSpinBox() const noexcept { return m_subtitleFontSizeSpinBox; }

    // Page 5: Associations Controls
    [[nodiscard]] QPushButton* selectAllExtensionsButton() const noexcept { return m_selectAllExtensionsBtn; }
    [[nodiscard]] QPushButton* deselectAllExtensionsButton() const noexcept { return m_deselectAllExtensionsBtn; }
    [[nodiscard]] QList<QCheckBox*> extensionCheckBoxes() const { return m_extensionCheckBoxes; }

    // Page 6: Hotkeys Controls
    [[nodiscard]] QTableWidget* hotkeysTable() const noexcept { return m_hotkeysTable; }
    [[nodiscard]] QLineEdit* hotkeyFilterEdit() const noexcept { return m_hotkeyFilterEdit; }
    [[nodiscard]] QPushButton* resetHotkeysButton() const noexcept { return m_resetHotkeysButton; }

signals:
    void settingsApplied();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void onHotkeyFilterChanged(const QString& filter);

private:
    void setupUi();
    void setupCategoryTree();
    void setupPages();
    QWidget* createGeneralPage();
    QWidget* createPlaybackPage();
    QWidget* createVideoPage();
    QWidget* createAudioPage();
    QWidget* createSubtitlesPage();
    QWidget* createAssociationsPage();
    QWidget* createHotkeysPage();

    void applyDarkTheme();

    core::SettingsManager* m_settings{nullptr};

    // Main layout elements
    QLineEdit* m_searchEdit{nullptr};
    QTreeWidget* m_categoryTree{nullptr};
    QStackedWidget* m_stackedWidget{nullptr};
    QPushButton* m_resetDefaultsButton{nullptr};
    QPushButton* m_okButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
    QPushButton* m_applyButton{nullptr};

    // General page
    QCheckBox* m_singleInstanceCheckBox{nullptr};
    QCheckBox* m_minimizeToTrayCheckBox{nullptr};
    QComboBox* m_accentColorComboBox{nullptr};
    QSlider* m_opacitySlider{nullptr};
    QLabel* m_opacityValueLabel{nullptr};

    // Playback page
    QDoubleSpinBox* m_speedSpinBox{nullptr};
    QSpinBox* m_shortJumpSpinBox{nullptr};
    QSpinBox* m_mediumJumpSpinBox{nullptr};
    QSpinBox* m_largeJumpSpinBox{nullptr};
    QCheckBox* m_autoResumeCheckBox{nullptr};

    // Video page
    QComboBox* m_hwdecComboBox{nullptr};
    QComboBox* m_aspectRatioComboBox{nullptr};
    QCheckBox* m_deinterlaceCheckBox{nullptr};

    // Audio page
    QComboBox* m_audioDeviceComboBox{nullptr};
    QCheckBox* m_normalizerCheckBox{nullptr};
    QDoubleSpinBox* m_audioDelayStepSpinBox{nullptr};

    // Subtitles page
    QLineEdit* m_subtitleLanguageEdit{nullptr};
    QSpinBox* m_subtitleFontSizeSpinBox{nullptr};

    // Associations page
    QPushButton* m_selectAllExtensionsBtn{nullptr};
    QPushButton* m_deselectAllExtensionsBtn{nullptr};
    QList<QCheckBox*> m_extensionCheckBoxes;

    // Hotkeys page
    QLineEdit* m_hotkeyFilterEdit{nullptr};
    QTableWidget* m_hotkeysTable{nullptr};
    QPushButton* m_resetHotkeysButton{nullptr};

    // Snapshot of settings loaded at open for Cancel
    struct SettingsSnapshot {
        bool singleInstance{false};
        bool minimizeToTray{false};
        QString accentColor{QStringLiteral("Amber")};
        int opacity{100};
        double defaultSpeed{1.0};
        int shortJump{5};
        int mediumJump{30};
        int largeJump{60};
        bool autoResume{true};
        QString hwdec{QStringLiteral("auto")};
        QString aspectRatio{QStringLiteral("auto")};
        bool deinterlace{false};
        QString audioDevice{QStringLiteral("Auto")};
        bool normalizer{false};
        double audioDelayStep{0.05};
        QString subtitleLanguage{QStringLiteral("en")};
        int subtitleFontSize{24};
        QList<bool> extensionStates;
    } m_snapshot;
};

} // namespace outplayer::ui

#endif // OUTPLAYER_UI_PREFERENCESDIALOG_H
