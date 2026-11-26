// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/PreferencesDialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "core/CommandRegistry.h"
#include "core/SettingsManager.h"
#include "engine/IPlaybackEngine.h"
#include "ui/ThemeManager.h"

namespace outplayer::ui {

PreferencesDialog::PreferencesDialog(core::SettingsManager* settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings ? settings : &core::SettingsManager::instance())
{
    setWindowTitle(tr("Preferences"));
    resize(680, 520);
    setMinimumSize(640, 480);

    setupUi();
    applyDarkTheme();
    loadSettings();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &PreferencesDialog::applyDarkTheme);
}

void PreferencesDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Top Search Bar
    auto* searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(6);
    auto* searchLabel = new QLabel(tr("Search:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search preferences..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &PreferencesDialog::onSearchTextChanged);

    // Content Splitter: Tree on left, StackedWidget on right
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    setupCategoryTree();
    splitter->addWidget(m_categoryTree);

    setupPages();
    splitter->addWidget(m_stackedWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    // Bottom Action Buttons
    auto* buttonLayout = new QHBoxLayout();
    m_resetDefaultsButton = new QPushButton(tr("Reset Defaults"), this);
    buttonLayout->addWidget(m_resetDefaultsButton);

    buttonLayout->addStretch(1);

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_applyButton = new QPushButton(tr("Apply"), this);

    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_resetDefaultsButton, &QPushButton::clicked, this, &PreferencesDialog::resetDefaults);
    connect(m_okButton, &QPushButton::clicked, this, &PreferencesDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &PreferencesDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &PreferencesDialog::saveSettings);
}

void PreferencesDialog::setupCategoryTree()
{
    m_categoryTree = new QTreeWidget(this);
    m_categoryTree->setHeaderHidden(true);
    m_categoryTree->setRootIsDecorated(true);
    m_categoryTree->setAnimated(true);

    struct CategoryDef {
        QString name;
        QStringList subcategories;
    };

    const QList<CategoryDef> categories = {
        {tr("General"), {tr("Startup"), tr("Skin/Theme"), tr("OSD"), tr("Window")}},
        {tr("Playback"), {tr("Speed"), tr("Seeking/Jump"), tr("Resume")}},
        {tr("Video"), {tr("Hardware Decoding"), tr("Deinterlacing"), tr("Aspect Ratio")}},
        {tr("Audio"), {tr("Output Device"), tr("Normalizer"), tr("Equalizer")}},
        {tr("Subtitles"), {tr("Language"), tr("Font"), tr("Size"), tr("Position")}},
        {tr("Associations"), {tr("Video Formats"), tr("Audio Formats"), tr("Playlists")}},
        {tr("Hotkeys"), {tr("Shortcuts Table"), tr("Rebind"), tr("Reset Defaults")}}
    };

    for (int i = 0; i < categories.size(); ++i) {
        auto* topItem = new QTreeWidgetItem(m_categoryTree);
        topItem->setText(0, categories[i].name);
        topItem->setData(0, Qt::UserRole, i);

        for (const auto& sub : categories[i].subcategories) {
            auto* childItem = new QTreeWidgetItem(topItem);
            childItem->setText(0, sub);
            childItem->setData(0, Qt::UserRole, i);
        }
        topItem->setExpanded(true);
    }

    connect(m_categoryTree, &QTreeWidget::currentItemChanged, this, &PreferencesDialog::onTreeCurrentItemChanged);
}

void PreferencesDialog::setupPages()
{
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(createGeneralPage());      // Index 0
    m_stackedWidget->addWidget(createPlaybackPage());     // Index 1
    m_stackedWidget->addWidget(createVideoPage());        // Index 2
    m_stackedWidget->addWidget(createAudioPage());        // Index 3
    m_stackedWidget->addWidget(createSubtitlesPage());    // Index 4
    m_stackedWidget->addWidget(createAssociationsPage()); // Index 5
    m_stackedWidget->addWidget(createHotkeysPage());      // Index 6
}

QWidget* PreferencesDialog::createGeneralPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* instanceGroup = new QGroupBox(tr("Instance & System"), page);
    auto* instanceLayout = new QVBoxLayout(instanceGroup);
    m_singleInstanceCheckBox = new QCheckBox(tr("Single Instance Reuse Window"), instanceGroup);
    m_minimizeToTrayCheckBox = new QCheckBox(tr("Minimize to System Tray"), instanceGroup);
    instanceLayout->addWidget(m_singleInstanceCheckBox);
    instanceLayout->addWidget(m_minimizeToTrayCheckBox);
    layout->addWidget(instanceGroup);

    auto* appearanceGroup = new QGroupBox(tr("Appearance & Theme"), page);
    auto* formLayout = new QFormLayout(appearanceGroup);
    formLayout->setSpacing(10);

    m_accentColorComboBox = new QComboBox(appearanceGroup);
    m_accentColorComboBox->addItems({tr("Amber"), tr("Blue"), tr("Green"), tr("Crimson")});
    formLayout->addRow(tr("Accent Color:"), m_accentColorComboBox);

    auto* opacityRow = new QHBoxLayout();
    m_opacitySlider = new QSlider(Qt::Horizontal, appearanceGroup);
    m_opacitySlider->setRange(20, 100);
    m_opacitySlider->setValue(100);
    m_opacityValueLabel = new QLabel(QStringLiteral("100%"), appearanceGroup);
    m_opacityValueLabel->setFixedWidth(40);
    opacityRow->addWidget(m_opacitySlider);
    opacityRow->addWidget(m_opacityValueLabel);
    formLayout->addRow(tr("Window Opacity:"), opacityRow);

    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int val) {
        m_opacityValueLabel->setText(QString::number(val) + QStringLiteral("%"));
    });

    layout->addWidget(appearanceGroup);
    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createPlaybackPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* speedGroup = new QGroupBox(tr("Speed & Seeking"), page);
    auto* formLayout = new QFormLayout(speedGroup);
    formLayout->setSpacing(10);

    m_speedSpinBox = new QDoubleSpinBox(speedGroup);
    m_speedSpinBox->setRange(0.2, 5.0);
    m_speedSpinBox->setSingleStep(0.05);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSuffix(QStringLiteral("x"));
    formLayout->addRow(tr("Default Speed:"), m_speedSpinBox);

    m_shortJumpSpinBox = new QSpinBox(speedGroup);
    m_shortJumpSpinBox->setRange(1, 60);
    m_shortJumpSpinBox->setValue(5);
    m_shortJumpSpinBox->setSuffix(QStringLiteral(" s"));
    formLayout->addRow(tr("Short Jump Step:"), m_shortJumpSpinBox);

    m_mediumJumpSpinBox = new QSpinBox(speedGroup);
    m_mediumJumpSpinBox->setRange(5, 300);
    m_mediumJumpSpinBox->setValue(30);
    m_mediumJumpSpinBox->setSuffix(QStringLiteral(" s"));
    formLayout->addRow(tr("Medium Jump Step:"), m_mediumJumpSpinBox);

    m_largeJumpSpinBox = new QSpinBox(speedGroup);
    m_largeJumpSpinBox->setRange(10, 600);
    m_largeJumpSpinBox->setValue(60);
    m_largeJumpSpinBox->setSuffix(QStringLiteral(" s"));
    formLayout->addRow(tr("Large Jump Step:"), m_largeJumpSpinBox);

    layout->addWidget(speedGroup);

    auto* resumeGroup = new QGroupBox(tr("Playback State"), page);
    auto* resumeLayout = new QVBoxLayout(resumeGroup);
    m_autoResumeCheckBox = new QCheckBox(tr("Auto-resume playback from last position"), resumeGroup);
    m_autoResumeCheckBox->setChecked(true);
    resumeLayout->addWidget(m_autoResumeCheckBox);
    layout->addWidget(resumeGroup);

    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createVideoPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* renderGroup = new QGroupBox(tr("Rendering & Decoder"), page);
    auto* formLayout = new QFormLayout(renderGroup);
    formLayout->setSpacing(10);

    m_hwdecComboBox = new QComboBox(renderGroup);
    m_hwdecComboBox->addItems({
        QStringLiteral("auto-safe"),
        QStringLiteral("auto"),
        QStringLiteral("vaapi"),
        QStringLiteral("nvdec"),
        QStringLiteral("vdpau"),
        QStringLiteral("no")
    });
    formLayout->addRow(tr("Hardware Decoding:"), m_hwdecComboBox);

    m_aspectRatioComboBox = new QComboBox(renderGroup);
    m_aspectRatioComboBox->addItems({
        QStringLiteral("auto"),
        QStringLiteral("16:9"),
        QStringLiteral("4:3"),
        QStringLiteral("2.35:1")
    });
    formLayout->addRow(tr("Default Aspect Ratio:"), m_aspectRatioComboBox);

    m_deinterlaceCheckBox = new QCheckBox(tr("Enable Deinterlacing"), renderGroup);
    formLayout->addRow(m_deinterlaceCheckBox);

    layout->addWidget(renderGroup);
    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createAudioPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* audioGroup = new QGroupBox(tr("Audio Processing & Output"), page);
    auto* formLayout = new QFormLayout(audioGroup);
    formLayout->setSpacing(10);

    m_audioDeviceComboBox = new QComboBox(audioGroup);
    m_audioDeviceComboBox->addItems({
        tr("Auto (System Default)"),
        QStringLiteral("PulseAudio / PipeWire"),
        QStringLiteral("ALSA Default")
    });
    formLayout->addRow(tr("Output Device:"), m_audioDeviceComboBox);

    m_normalizerCheckBox = new QCheckBox(tr("Enable Audio Normalizer / Volume Compressor"), audioGroup);
    formLayout->addRow(m_normalizerCheckBox);

    m_audioDelayStepSpinBox = new QDoubleSpinBox(audioGroup);
    m_audioDelayStepSpinBox->setRange(0.01, 1.0);
    m_audioDelayStepSpinBox->setSingleStep(0.01);
    m_audioDelayStepSpinBox->setValue(0.05);
    m_audioDelayStepSpinBox->setSuffix(QStringLiteral(" s"));
    formLayout->addRow(tr("Audio Delay Step:"), m_audioDelayStepSpinBox);

    layout->addWidget(audioGroup);
    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createSubtitlesPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto* subGroup = new QGroupBox(tr("Subtitle Appearance & Language"), page);
    auto* formLayout = new QFormLayout(subGroup);
    formLayout->setSpacing(10);

    m_subtitleLanguageEdit = new QLineEdit(subGroup);
    m_subtitleLanguageEdit->setPlaceholderText(tr("e.g., en, ja, es, ko"));
    formLayout->addRow(tr("Default Language:"), m_subtitleLanguageEdit);

    m_subtitleFontCombo = new QFontComboBox(subGroup);
    connect(m_subtitleFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont&) {
        updateSubtitlePreview();
    });
    formLayout->addRow(tr("Font Family:"), m_subtitleFontCombo);

    m_subtitleFontSizeSpinBox = new QSpinBox(subGroup);
    m_subtitleFontSizeSpinBox->setRange(10, 96);
    m_subtitleFontSizeSpinBox->setValue(24);
    m_subtitleFontSizeSpinBox->setSuffix(QStringLiteral(" pt"));
    connect(m_subtitleFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateSubtitlePreview();
    });
    formLayout->addRow(tr("Font Size:"), m_subtitleFontSizeSpinBox);

    auto* styleRow = new QHBoxLayout();
    m_subtitleBoldCheck = new QCheckBox(tr("Bold"), subGroup);
    m_subtitleItalicCheck = new QCheckBox(tr("Italic"), subGroup);
    styleRow->addWidget(m_subtitleBoldCheck);
    styleRow->addWidget(m_subtitleItalicCheck);
    styleRow->addStretch(1);
    connect(m_subtitleBoldCheck, &QCheckBox::toggled, this, [this](bool) { updateSubtitlePreview(); });
    connect(m_subtitleItalicCheck, &QCheckBox::toggled, this, [this](bool) { updateSubtitlePreview(); });
    formLayout->addRow(tr("Font Style:"), styleRow);

    // Text Color
    m_subtitleColorBtn = new QPushButton(subGroup);
    m_subtitleColorBtn->setFixedWidth(100);
    updateColorButtonSwatch(m_subtitleColorBtn, m_subtitleColor);
    connect(m_subtitleColorBtn, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(m_subtitleColor, this, tr("Select Subtitle Text Color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setSubtitleColor(c);
        }
    });
    formLayout->addRow(tr("Text Color:"), m_subtitleColorBtn);

    // Border Color and Size
    auto* borderRow = new QHBoxLayout();
    m_subtitleBorderColorBtn = new QPushButton(subGroup);
    m_subtitleBorderColorBtn->setFixedWidth(100);
    updateColorButtonSwatch(m_subtitleBorderColorBtn, m_subtitleBorderColor);
    connect(m_subtitleBorderColorBtn, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(m_subtitleBorderColor, this, tr("Select Subtitle Border Color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setSubtitleBorderColor(c);
        }
    });
    borderRow->addWidget(m_subtitleBorderColorBtn);

    auto* borderSizeLabel = new QLabel(tr("Size:"), subGroup);
    borderRow->addWidget(borderSizeLabel);
    m_subtitleBorderSizeSpinBox = new QDoubleSpinBox(subGroup);
    m_subtitleBorderSizeSpinBox->setRange(0.0, 10.0);
    m_subtitleBorderSizeSpinBox->setSingleStep(0.5);
    m_subtitleBorderSizeSpinBox->setValue(2.0);
    m_subtitleBorderSizeSpinBox->setSuffix(QStringLiteral(" px"));
    connect(m_subtitleBorderSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        updateSubtitlePreview();
    });
    borderRow->addWidget(m_subtitleBorderSizeSpinBox);
    borderRow->addStretch(1);
    formLayout->addRow(tr("Border:"), borderRow);

    // Shadow Color and Offset
    auto* shadowRow = new QHBoxLayout();
    m_subtitleShadowColorBtn = new QPushButton(subGroup);
    m_subtitleShadowColorBtn->setFixedWidth(100);
    updateColorButtonSwatch(m_subtitleShadowColorBtn, m_subtitleShadowColor);
    connect(m_subtitleShadowColorBtn, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(m_subtitleShadowColor, this, tr("Select Subtitle Shadow Color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setSubtitleShadowColor(c);
        }
    });
    shadowRow->addWidget(m_subtitleShadowColorBtn);

    auto* shadowOffsetLabel = new QLabel(tr("Offset:"), subGroup);
    shadowRow->addWidget(shadowOffsetLabel);
    m_subtitleShadowOffsetSpinBox = new QDoubleSpinBox(subGroup);
    m_subtitleShadowOffsetSpinBox->setRange(0.0, 10.0);
    m_subtitleShadowOffsetSpinBox->setSingleStep(0.5);
    m_subtitleShadowOffsetSpinBox->setValue(0.0);
    m_subtitleShadowOffsetSpinBox->setSuffix(QStringLiteral(" px"));
    connect(m_subtitleShadowOffsetSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        updateSubtitlePreview();
    });
    shadowRow->addWidget(m_subtitleShadowOffsetSpinBox);
    shadowRow->addStretch(1);
    formLayout->addRow(tr("Shadow:"), shadowRow);

    layout->addWidget(subGroup);

    // Live Preview box
    auto* previewGroup = new QGroupBox(tr("Live Preview"), page);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    m_subtitlePreviewLabel = new QLabel(tr("Sample Subtitle Text (00:01:23)"), previewGroup);
    m_subtitlePreviewLabel->setAlignment(Qt::AlignCenter);
    m_subtitlePreviewLabel->setMinimumHeight(64);
    previewLayout->addWidget(m_subtitlePreviewLabel);
    layout->addWidget(previewGroup);

    updateSubtitlePreview();

    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createAssociationsPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* topBtnLayout = new QHBoxLayout();
    m_selectAllExtensionsBtn = new QPushButton(tr("Select All"), page);
    m_deselectAllExtensionsBtn = new QPushButton(tr("Deselect All"), page);
    topBtnLayout->addWidget(m_selectAllExtensionsBtn);
    topBtnLayout->addWidget(m_deselectAllExtensionsBtn);
    topBtnLayout->addStretch(1);
    layout->addLayout(topBtnLayout);

    connect(m_selectAllExtensionsBtn, &QPushButton::clicked, this, [this]() {
        for (auto* cb : m_extensionCheckBoxes) {
            cb->setChecked(true);
        }
    });

    connect(m_deselectAllExtensionsBtn, &QPushButton::clicked, this, [this]() {
        for (auto* cb : m_extensionCheckBoxes) {
            cb->setChecked(false);
        }
    });

    auto* videoGroup = new QGroupBox(tr("Video Formats"), page);
    auto* videoLayout = new QGridLayout(videoGroup);
    QStringList videoExts = {QStringLiteral(".mp4"), QStringLiteral(".mkv"), QStringLiteral(".avi"),
                             QStringLiteral(".mov"), QStringLiteral(".wmv"), QStringLiteral(".flv"),
                             QStringLiteral(".webm"), QStringLiteral(".ts"), QStringLiteral(".m4v")};
    for (int i = 0; i < videoExts.size(); ++i) {
        auto* cb = new QCheckBox(videoExts[i], videoGroup);
        cb->setChecked(true);
        m_extensionCheckBoxes.append(cb);
        videoLayout->addWidget(cb, i / 3, i % 3);
    }
    layout->addWidget(videoGroup);

    auto* audioGroup = new QGroupBox(tr("Audio Formats"), page);
    auto* audioLayout = new QGridLayout(audioGroup);
    QStringList audioExts = {QStringLiteral(".mp3"), QStringLiteral(".flac"), QStringLiteral(".aac"),
                             QStringLiteral(".ogg"), QStringLiteral(".wav"), QStringLiteral(".m4a"),
                             QStringLiteral(".opus"), QStringLiteral(".wma")};
    for (int i = 0; i < audioExts.size(); ++i) {
        auto* cb = new QCheckBox(audioExts[i], audioGroup);
        cb->setChecked(true);
        m_extensionCheckBoxes.append(cb);
        audioLayout->addWidget(cb, i / 3, i % 3);
    }
    layout->addWidget(audioGroup);

    auto* playlistGroup = new QGroupBox(tr("Playlists"), page);
    auto* playlistLayout = new QGridLayout(playlistGroup);
    QStringList plExts = {QStringLiteral(".m3u"), QStringLiteral(".m3u8"),
                          QStringLiteral(".pls"), QStringLiteral(".cue")};
    for (int i = 0; i < plExts.size(); ++i) {
        auto* cb = new QCheckBox(plExts[i], playlistGroup);
        cb->setChecked(true);
        m_extensionCheckBoxes.append(cb);
        playlistLayout->addWidget(cb, i / 3, i % 3);
    }
    layout->addWidget(playlistGroup);

    layout->addStretch(1);
    return page;
}

QWidget* PreferencesDialog::createHotkeysPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* filterRow = new QHBoxLayout();
    auto* filterLabel = new QLabel(tr("Filter:"), page);
    m_hotkeyFilterEdit = new QLineEdit(page);
    m_hotkeyFilterEdit->setPlaceholderText(tr("Filter shortcuts..."));
    m_hotkeyFilterEdit->setClearButtonEnabled(true);
    filterRow->addWidget(filterLabel);
    filterRow->addWidget(m_hotkeyFilterEdit);
    layout->addLayout(filterRow);

    connect(m_hotkeyFilterEdit, &QLineEdit::textChanged, this, &PreferencesDialog::onHotkeyFilterChanged);

    m_hotkeysTable = new QTableWidget(page);
    m_hotkeysTable->setColumnCount(3);
    m_hotkeysTable->setHorizontalHeaderLabels({tr("Command ID"), tr("Description"), tr("Shortcut")});
    m_hotkeysTable->horizontalHeader()->setStretchLastSection(true);
    m_hotkeysTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_hotkeysTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_hotkeysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hotkeysTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_hotkeysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    const auto commands = core::CommandRegistry::instance().allCommands();
    m_hotkeysTable->setRowCount(commands.size());
    for (int r = 0; r < commands.size(); ++r) {
        const auto& cmd = commands[r];
        auto* idItem = new QTableWidgetItem(cmd.name);
        auto* descItem = new QTableWidgetItem(cmd.description);
        auto* keyItem = new QTableWidgetItem(cmd.defaultKey);

        m_hotkeysTable->setItem(r, 0, idItem);
        m_hotkeysTable->setItem(r, 1, descItem);
        m_hotkeysTable->setItem(r, 2, keyItem);
    }
    layout->addWidget(m_hotkeysTable, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    m_resetHotkeysButton = new QPushButton(tr("Reset Hotkeys"), page);
    btnRow->addWidget(m_resetHotkeysButton);
    layout->addLayout(btnRow);

    connect(m_resetHotkeysButton, &QPushButton::clicked, this, [this]() {
        const auto allCommands = core::CommandRegistry::instance().allCommands();
        for (int r = 0; r < allCommands.size() && r < m_hotkeysTable->rowCount(); ++r) {
            const auto& cmd = allCommands[r];
            if (auto* item = m_hotkeysTable->item(r, 2)) {
                item->setText(cmd.defaultKey);
            }
        }
    });

    return page;
}

void PreferencesDialog::onSearchTextChanged(const QString& text)
{
    const QString query = text.trimmed();
    const bool filtering = !query.isEmpty();

    for (int i = 0; i < m_categoryTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* topItem = m_categoryTree->topLevelItem(i);
        const bool topMatches = topItem->text(0).contains(query, Qt::CaseInsensitive);
        bool anyChildMatches = false;

        for (int c = 0; c < topItem->childCount(); ++c) {
            QTreeWidgetItem* child = topItem->child(c);
            const bool childMatches = child->text(0).contains(query, Qt::CaseInsensitive);
            if (childMatches) {
                anyChildMatches = true;
            }
            child->setHidden(filtering && !topMatches && !childMatches);
        }

        const bool showTop = !filtering || topMatches || anyChildMatches;
        topItem->setHidden(!showTop);
        if (anyChildMatches) {
            topItem->setExpanded(true);
        }
    }
}

void PreferencesDialog::onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
    if (!current) return;
    const int pageIndex = current->data(0, Qt::UserRole).toInt();
    if (pageIndex >= 0 && pageIndex < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(pageIndex);
    }
}

void PreferencesDialog::onHotkeyFilterChanged(const QString& filter)
{
    const QString q = filter.trimmed();
    for (int r = 0; r < m_hotkeysTable->rowCount(); ++r) {
        bool match = q.isEmpty();
        if (!match) {
            for (int col = 0; col < m_hotkeysTable->columnCount(); ++col) {
                auto* item = m_hotkeysTable->item(r, col);
                if (item && item->text().contains(q, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        m_hotkeysTable->setRowHidden(r, !match);
    }
}

void PreferencesDialog::selectCategory(const QString& categoryName)
{
    for (int i = 0; i < m_categoryTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* topItem = m_categoryTree->topLevelItem(i);
        if (topItem->text(0).compare(categoryName, Qt::CaseInsensitive) == 0) {
            m_categoryTree->setCurrentItem(topItem);
            return;
        }
        for (int c = 0; c < topItem->childCount(); ++c) {
            QTreeWidgetItem* child = topItem->child(c);
            if (child->text(0).compare(categoryName, Qt::CaseInsensitive) == 0) {
                m_categoryTree->setCurrentItem(child);
                return;
            }
        }
    }
}

void PreferencesDialog::setSearchFilter(const QString& query)
{
    m_searchEdit->setText(query);
}

void PreferencesDialog::loadSettings()
{
    // General
    const bool singleInst = m_settings->get(QStringLiteral("General/SingleInstance"), false).toBool();
    m_singleInstanceCheckBox->setChecked(singleInst);

    const bool minTray = m_settings->get(QStringLiteral("General/MinimizeToTray"), false).toBool();
    m_minimizeToTrayCheckBox->setChecked(minTray);

    const QString accent = m_settings->get(QStringLiteral("General/AccentColor"), QStringLiteral("Amber")).toString();
    const int accentIndex = m_accentColorComboBox->findText(accent, Qt::MatchFixedString);
    if (accentIndex >= 0) {
        m_accentColorComboBox->setCurrentIndex(accentIndex);
    }

    const int opacity = m_settings->get(QStringLiteral("General/WindowOpacity"), 100).toInt();
    m_opacitySlider->setValue(opacity);
    m_opacityValueLabel->setText(QString::number(opacity) + QStringLiteral("%"));

    // Playback
    const double speed = m_settings->get(QStringLiteral("Playback/DefaultSpeed"), 1.0).toDouble();
    m_speedSpinBox->setValue(speed);

    const int shortJump = m_settings->get(QStringLiteral("Playback/ShortJumpStep"), 5).toInt();
    m_shortJumpSpinBox->setValue(shortJump);

    const int medJump = m_settings->get(QStringLiteral("Playback/MediumJumpStep"), 30).toInt();
    m_mediumJumpSpinBox->setValue(medJump);

    const int largeJump = m_settings->get(QStringLiteral("Playback/LargeJumpStep"), 60).toInt();
    m_largeJumpSpinBox->setValue(largeJump);

    const bool autoResume = m_settings->get(QStringLiteral("Playback/AutoResume"), true).toBool();
    m_autoResumeCheckBox->setChecked(autoResume);

    // Video
    const QString hwdec = m_settings->get(QStringLiteral("Video/HardwareDecoder"), QStringLiteral("auto-safe")).toString();
    const int hwdecIndex = m_hwdecComboBox->findText(hwdec, Qt::MatchFixedString);
    if (hwdecIndex >= 0) {
        m_hwdecComboBox->setCurrentIndex(hwdecIndex);
    }

    const QString aspect = m_settings->get(QStringLiteral("Video/AspectRatio"), QStringLiteral("auto")).toString();
    const int aspectIndex = m_aspectRatioComboBox->findText(aspect, Qt::MatchFixedString);
    if (aspectIndex >= 0) {
        m_aspectRatioComboBox->setCurrentIndex(aspectIndex);
    }

    const QVariant deintVar = m_settings->contains(QStringLiteral("Video/Deinterlacing"))
        ? m_settings->get(QStringLiteral("Video/Deinterlacing"))
        : m_settings->get(QStringLiteral("Video/Deinterlace"), false);
    const QString deintStr = deintVar.toString().trimmed().toLower();
    const bool deint = deintVar.toBool() || deintStr == QStringLiteral("yes") || deintStr == QStringLiteral("true");
    m_deinterlaceCheckBox->setChecked(deint);

    // Audio
    const QString audioDev = m_settings->get(QStringLiteral("Audio/OutputDevice"), QStringLiteral("Auto")).toString();
    const int devIndex = m_audioDeviceComboBox->findText(audioDev, Qt::MatchFixedString);
    if (devIndex >= 0) {
        m_audioDeviceComboBox->setCurrentIndex(devIndex);
    }

    const bool norm = m_settings->get(QStringLiteral("Audio/Normalizer"), false).toBool();
    m_normalizerCheckBox->setChecked(norm);

    const double delayStep = m_settings->get(QStringLiteral("Audio/AudioDelayStep"), 0.05).toDouble();
    m_audioDelayStepSpinBox->setValue(delayStep);

    // Subtitles
    const QString subLang = m_settings->get(QStringLiteral("Subtitles/DefaultLanguage"), QStringLiteral("en")).toString();
    m_subtitleLanguageEdit->setText(subLang);

    const QString subFont = m_settings->get(QStringLiteral("Subtitles/Font"), QStringLiteral("sans-serif")).toString();
    if (m_subtitleFontCombo) {
        m_subtitleFontCombo->setCurrentFont(QFont(subFont));
    }

    const int subFontSize = m_settings->get(QStringLiteral("Subtitles/FontSize"), 24).toInt();
    m_subtitleFontSizeSpinBox->setValue(subFontSize);

    const QString colorStr = m_settings->get(QStringLiteral("Subtitles/Color"), QStringLiteral("#ffffffff")).toString();
    setSubtitleColor(QColor::isValidColorName(colorStr) ? QColor(colorStr) : QColor(255, 255, 255, 255));

    const bool subBold = m_settings->get(QStringLiteral("Subtitles/Bold"), false).toBool();
    if (m_subtitleBoldCheck) {
        m_subtitleBoldCheck->setChecked(subBold);
    }

    const bool subItalic = m_settings->get(QStringLiteral("Subtitles/Italic"), false).toBool();
    if (m_subtitleItalicCheck) {
        m_subtitleItalicCheck->setChecked(subItalic);
    }

    const QString borderColStr = m_settings->get(QStringLiteral("Subtitles/BorderColor"), QStringLiteral("#ff000000")).toString();
    setSubtitleBorderColor(QColor::isValidColorName(borderColStr) ? QColor(borderColStr) : QColor(0, 0, 0, 255));

    const double borderSize = m_settings->get(QStringLiteral("Subtitles/BorderSize"), 2.0).toDouble();
    if (m_subtitleBorderSizeSpinBox) {
        m_subtitleBorderSizeSpinBox->setValue(borderSize);
    }

    const QString shadowColStr = m_settings->get(QStringLiteral("Subtitles/ShadowColor"), QStringLiteral("#80000000")).toString();
    setSubtitleShadowColor(QColor::isValidColorName(shadowColStr) ? QColor(shadowColStr) : QColor(0, 0, 0, 128));

    const double shadowOffset = m_settings->get(QStringLiteral("Subtitles/ShadowOffset"), 0.0).toDouble();
    if (m_subtitleShadowOffsetSpinBox) {
        m_subtitleShadowOffsetSpinBox->setValue(shadowOffset);
    }

    updateSubtitlePreview();

    // Take snapshot for cancel
    m_snapshot.singleInstance = singleInst;
    m_snapshot.minimizeToTray = minTray;
    m_snapshot.accentColor = accent;
    m_snapshot.opacity = opacity;
    m_snapshot.defaultSpeed = speed;
    m_snapshot.shortJump = shortJump;
    m_snapshot.mediumJump = medJump;
    m_snapshot.largeJump = largeJump;
    m_snapshot.autoResume = autoResume;
    m_snapshot.hwdec = hwdec;
    m_snapshot.aspectRatio = aspect;
    m_snapshot.deinterlace = deint;
    m_snapshot.audioDevice = audioDev;
    m_snapshot.normalizer = norm;
    m_snapshot.audioDelayStep = delayStep;
    m_snapshot.subtitleLanguage = subLang;
    m_snapshot.subtitleFont = subFont;
    m_snapshot.subtitleFontSize = subFontSize;
    m_snapshot.subtitleColor = m_subtitleColor;
    m_snapshot.subtitleBold = subBold;
    m_snapshot.subtitleItalic = subItalic;
    m_snapshot.subtitleBorderColor = m_subtitleBorderColor;
    m_snapshot.subtitleBorderSize = borderSize;
    m_snapshot.subtitleShadowColor = m_subtitleShadowColor;
    m_snapshot.subtitleShadowOffset = shadowOffset;

    m_snapshot.extensionStates.clear();
    for (auto* cb : m_extensionCheckBoxes) {
        m_snapshot.extensionStates.append(cb->isChecked());
    }
}

void PreferencesDialog::saveSettings()
{
    // General
    const bool singleInst = m_singleInstanceCheckBox->isChecked();
    m_settings->set(QStringLiteral("General/SingleInstance"), singleInst);
    m_settings->set(QStringLiteral("General/AllowMultipleInstances"), !singleInst);

    const bool minTray = m_minimizeToTrayCheckBox->isChecked();
    m_settings->set(QStringLiteral("General/MinimizeToTray"), minTray);

    const QString accent = m_accentColorComboBox->currentText();
    m_settings->set(QStringLiteral("General/AccentColor"), accent);

    const int opacity = m_opacitySlider->value();
    m_settings->set(QStringLiteral("General/WindowOpacity"), opacity);

    // Playback
    m_settings->set(QStringLiteral("Playback/DefaultSpeed"), m_speedSpinBox->value());
    m_settings->set(QStringLiteral("Playback/ShortJumpStep"), m_shortJumpSpinBox->value());
    m_settings->set(QStringLiteral("Playback/MediumJumpStep"), m_mediumJumpSpinBox->value());
    m_settings->set(QStringLiteral("Playback/LargeJumpStep"), m_largeJumpSpinBox->value());
    m_settings->set(QStringLiteral("Playback/AutoResume"), m_autoResumeCheckBox->isChecked());

    // Video
    m_settings->set(QStringLiteral("Video/HardwareDecoder"), m_hwdecComboBox->currentText());
    m_settings->set(QStringLiteral("Video/AspectRatio"), m_aspectRatioComboBox->currentText());
    const bool deinterlaceEnabled = m_deinterlaceCheckBox->isChecked();
    m_settings->set(QStringLiteral("Video/Deinterlacing"), deinterlaceEnabled);
    m_settings->set(QStringLiteral("Video/Deinterlace"), deinterlaceEnabled ? QStringLiteral("yes") : QStringLiteral("auto"));

    // Audio
    m_settings->set(QStringLiteral("Audio/OutputDevice"), m_audioDeviceComboBox->currentText());
    m_settings->set(QStringLiteral("Audio/Normalizer"), m_normalizerCheckBox->isChecked());
    m_settings->set(QStringLiteral("Audio/AudioDelayStep"), m_audioDelayStepSpinBox->value());

    // Subtitles
    m_settings->set(QStringLiteral("Subtitles/DefaultLanguage"), m_subtitleLanguageEdit->text());
    const QString subFont = m_subtitleFontCombo ? m_subtitleFontCombo->currentFont().family() : QStringLiteral("sans-serif");
    m_settings->set(QStringLiteral("Subtitles/Font"), subFont);
    m_settings->set(QStringLiteral("Subtitles/FontSize"), m_subtitleFontSizeSpinBox->value());
    m_settings->set(QStringLiteral("Subtitles/Color"), m_subtitleColor.name(QColor::HexArgb));
    m_settings->set(QStringLiteral("Subtitles/Bold"), m_subtitleBoldCheck ? m_subtitleBoldCheck->isChecked() : false);
    m_settings->set(QStringLiteral("Subtitles/Italic"), m_subtitleItalicCheck ? m_subtitleItalicCheck->isChecked() : false);
    m_settings->set(QStringLiteral("Subtitles/BorderColor"), m_subtitleBorderColor.name(QColor::HexArgb));
    m_settings->set(QStringLiteral("Subtitles/BorderSize"), m_subtitleBorderSizeSpinBox ? m_subtitleBorderSizeSpinBox->value() : 2.0);
    m_settings->set(QStringLiteral("Subtitles/ShadowColor"), m_subtitleShadowColor.name(QColor::HexArgb));
    m_settings->set(QStringLiteral("Subtitles/ShadowOffset"), m_subtitleShadowOffsetSpinBox ? m_subtitleShadowOffsetSpinBox->value() : 0.0);

    if (m_engine) {
        m_engine->setHardwareDecoder(m_hwdecComboBox->currentText());
        m_engine->setAspectRatio(m_aspectRatioComboBox->currentText());
        m_engine->setDeinterlace(deinterlaceEnabled);
        m_engine->setNormalizerEnabled(m_normalizerCheckBox->isChecked());
        m_engine->setSpeed(m_speedSpinBox->value());
        m_engine->setSubtitleLanguage(m_subtitleLanguageEdit->text());
        m_engine->setSubtitleFont(subFont);
        m_engine->setSubtitleFontSize(m_subtitleFontSizeSpinBox->value());
        m_engine->setSubtitleColor(m_subtitleColor);
        if (m_subtitleBoldCheck) {
            m_engine->setSubtitleBold(m_subtitleBoldCheck->isChecked());
        }
        if (m_subtitleItalicCheck) {
            m_engine->setSubtitleItalic(m_subtitleItalicCheck->isChecked());
        }
        m_engine->setSubtitleBorderColor(m_subtitleBorderColor);
        if (m_subtitleBorderSizeSpinBox) {
            m_engine->setSubtitleBorderSize(m_subtitleBorderSizeSpinBox->value());
        }
        m_engine->setSubtitleShadowColor(m_subtitleShadowColor);
        if (m_subtitleShadowOffsetSpinBox) {
            m_engine->setSubtitleShadowOffset(m_subtitleShadowOffsetSpinBox->value());
        }
    }

    m_settings->sync();

    // Update snapshot
    m_snapshot.singleInstance = singleInst;
    m_snapshot.minimizeToTray = minTray;
    m_snapshot.accentColor = accent;
    m_snapshot.opacity = opacity;
    m_snapshot.defaultSpeed = m_speedSpinBox->value();
    m_snapshot.shortJump = m_shortJumpSpinBox->value();
    m_snapshot.mediumJump = m_mediumJumpSpinBox->value();
    m_snapshot.largeJump = m_largeJumpSpinBox->value();
    m_snapshot.autoResume = m_autoResumeCheckBox->isChecked();
    m_snapshot.hwdec = m_hwdecComboBox->currentText();
    m_snapshot.aspectRatio = m_aspectRatioComboBox->currentText();
    m_snapshot.deinterlace = m_deinterlaceCheckBox->isChecked();
    m_snapshot.audioDevice = m_audioDeviceComboBox->currentText();
    m_snapshot.normalizer = m_normalizerCheckBox->isChecked();
    m_snapshot.audioDelayStep = m_audioDelayStepSpinBox->value();
    m_snapshot.subtitleLanguage = m_subtitleLanguageEdit->text();
    m_snapshot.subtitleFont = subFont;
    m_snapshot.subtitleFontSize = m_subtitleFontSizeSpinBox->value();
    m_snapshot.subtitleColor = m_subtitleColor;
    m_snapshot.subtitleBold = m_subtitleBoldCheck ? m_subtitleBoldCheck->isChecked() : false;
    m_snapshot.subtitleItalic = m_subtitleItalicCheck ? m_subtitleItalicCheck->isChecked() : false;
    m_snapshot.subtitleBorderColor = m_subtitleBorderColor;
    m_snapshot.subtitleBorderSize = m_subtitleBorderSizeSpinBox ? m_subtitleBorderSizeSpinBox->value() : 2.0;
    m_snapshot.subtitleShadowColor = m_subtitleShadowColor;
    m_snapshot.subtitleShadowOffset = m_subtitleShadowOffsetSpinBox ? m_subtitleShadowOffsetSpinBox->value() : 0.0;

    m_snapshot.extensionStates.clear();
    for (auto* cb : m_extensionCheckBoxes) {
        m_snapshot.extensionStates.append(cb->isChecked());
    }

    emit settingsApplied();
}

void PreferencesDialog::resetDefaults()
{
    m_singleInstanceCheckBox->setChecked(false);
    m_minimizeToTrayCheckBox->setChecked(false);
    m_accentColorComboBox->setCurrentText(QStringLiteral("Amber"));
    m_opacitySlider->setValue(100);

    m_speedSpinBox->setValue(1.0);
    m_shortJumpSpinBox->setValue(5);
    m_mediumJumpSpinBox->setValue(30);
    m_largeJumpSpinBox->setValue(60);
    m_autoResumeCheckBox->setChecked(true);

    m_hwdecComboBox->setCurrentText(QStringLiteral("auto"));
    m_aspectRatioComboBox->setCurrentText(QStringLiteral("auto"));
    m_deinterlaceCheckBox->setChecked(false);

    m_audioDeviceComboBox->setCurrentIndex(0);
    m_normalizerCheckBox->setChecked(false);
    m_audioDelayStepSpinBox->setValue(0.05);

    m_subtitleLanguageEdit->setText(QStringLiteral("en"));
    if (m_subtitleFontCombo) {
        m_subtitleFontCombo->setCurrentFont(QFont(QStringLiteral("sans-serif")));
    }
    m_subtitleFontSizeSpinBox->setValue(24);
    if (m_subtitleBoldCheck) {
        m_subtitleBoldCheck->setChecked(false);
    }
    if (m_subtitleItalicCheck) {
        m_subtitleItalicCheck->setChecked(false);
    }
    setSubtitleColor(QColor(255, 255, 255, 255));
    setSubtitleBorderColor(QColor(0, 0, 0, 255));
    if (m_subtitleBorderSizeSpinBox) {
        m_subtitleBorderSizeSpinBox->setValue(2.0);
    }
    setSubtitleShadowColor(QColor(0, 0, 0, 128));
    if (m_subtitleShadowOffsetSpinBox) {
        m_subtitleShadowOffsetSpinBox->setValue(0.0);
    }
    updateSubtitlePreview();

    for (auto* cb : m_extensionCheckBoxes) {
        cb->setChecked(true);
    }
}

void PreferencesDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void PreferencesDialog::reject()
{
    // Revert controls to snapshot
    m_singleInstanceCheckBox->setChecked(m_snapshot.singleInstance);
    m_minimizeToTrayCheckBox->setChecked(m_snapshot.minimizeToTray);
    const int accentIndex = m_accentColorComboBox->findText(m_snapshot.accentColor, Qt::MatchFixedString);
    if (accentIndex >= 0) m_accentColorComboBox->setCurrentIndex(accentIndex);
    m_opacitySlider->setValue(m_snapshot.opacity);

    m_speedSpinBox->setValue(m_snapshot.defaultSpeed);
    m_shortJumpSpinBox->setValue(m_snapshot.shortJump);
    m_mediumJumpSpinBox->setValue(m_snapshot.mediumJump);
    m_largeJumpSpinBox->setValue(m_snapshot.largeJump);
    m_autoResumeCheckBox->setChecked(m_snapshot.autoResume);

    const int hwdecIndex = m_hwdecComboBox->findText(m_snapshot.hwdec, Qt::MatchFixedString);
    if (hwdecIndex >= 0) m_hwdecComboBox->setCurrentIndex(hwdecIndex);
    const int aspectIndex = m_aspectRatioComboBox->findText(m_snapshot.aspectRatio, Qt::MatchFixedString);
    if (aspectIndex >= 0) m_aspectRatioComboBox->setCurrentIndex(aspectIndex);
    m_deinterlaceCheckBox->setChecked(m_snapshot.deinterlace);

    const int devIndex = m_audioDeviceComboBox->findText(m_snapshot.audioDevice, Qt::MatchFixedString);
    if (devIndex >= 0) m_audioDeviceComboBox->setCurrentIndex(devIndex);
    m_normalizerCheckBox->setChecked(m_snapshot.normalizer);
    m_audioDelayStepSpinBox->setValue(m_snapshot.audioDelayStep);

    m_subtitleLanguageEdit->setText(m_snapshot.subtitleLanguage);
    if (m_subtitleFontCombo) {
        m_subtitleFontCombo->setCurrentFont(QFont(m_snapshot.subtitleFont));
    }
    m_subtitleFontSizeSpinBox->setValue(m_snapshot.subtitleFontSize);
    setSubtitleColor(m_snapshot.subtitleColor);
    if (m_subtitleBoldCheck) {
        m_subtitleBoldCheck->setChecked(m_snapshot.subtitleBold);
    }
    if (m_subtitleItalicCheck) {
        m_subtitleItalicCheck->setChecked(m_snapshot.subtitleItalic);
    }
    setSubtitleBorderColor(m_snapshot.subtitleBorderColor);
    if (m_subtitleBorderSizeSpinBox) {
        m_subtitleBorderSizeSpinBox->setValue(m_snapshot.subtitleBorderSize);
    }
    setSubtitleShadowColor(m_snapshot.subtitleShadowColor);
    if (m_subtitleShadowOffsetSpinBox) {
        m_subtitleShadowOffsetSpinBox->setValue(m_snapshot.subtitleShadowOffset);
    }
    updateSubtitlePreview();

    for (int i = 0; i < m_extensionCheckBoxes.size() && i < m_snapshot.extensionStates.size(); ++i) {
        m_extensionCheckBoxes[i]->setChecked(m_snapshot.extensionStates[i]);
    }

    QDialog::reject();
}

void PreferencesDialog::setSubtitleColor(const QColor& color)
{
    if (!color.isValid()) return;
    m_subtitleColor = color;
    updateColorButtonSwatch(m_subtitleColorBtn, m_subtitleColor);
    updateSubtitlePreview();
}

void PreferencesDialog::setSubtitleBorderColor(const QColor& color)
{
    if (!color.isValid()) return;
    m_subtitleBorderColor = color;
    updateColorButtonSwatch(m_subtitleBorderColorBtn, m_subtitleBorderColor);
    updateSubtitlePreview();
}

void PreferencesDialog::setSubtitleShadowColor(const QColor& color)
{
    if (!color.isValid()) return;
    m_subtitleShadowColor = color;
    updateColorButtonSwatch(m_subtitleShadowColorBtn, m_subtitleShadowColor);
    updateSubtitlePreview();
}

void PreferencesDialog::updateColorButtonSwatch(QPushButton* btn, const QColor& color)
{
    if (!btn) return;
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid #555555; border-radius: 3px; padding: 4px 10px; font-weight: bold; }"
    ).arg(color.name()).arg(color.lightness() > 128 ? QStringLiteral("#000000") : QStringLiteral("#ffffff")));
    btn->setText(color.name().toUpper());
}

void PreferencesDialog::updateSubtitlePreview()
{
    if (!m_subtitlePreviewLabel) return;
    QFont font = m_subtitleFontCombo ? m_subtitleFontCombo->currentFont() : QFont();
    int ptSize = m_subtitleFontSizeSpinBox ? m_subtitleFontSizeSpinBox->value() : 24;
    font.setPointSize(std::clamp(ptSize, 12, 32));
    font.setBold(m_subtitleBoldCheck && m_subtitleBoldCheck->isChecked());
    font.setItalic(m_subtitleItalicCheck && m_subtitleItalicCheck->isChecked());
    m_subtitlePreviewLabel->setFont(font);

    const QString weightStr = (m_subtitleBoldCheck && m_subtitleBoldCheck->isChecked()) ? QStringLiteral("bold") : QStringLiteral("normal");
    const QString italicStr = (m_subtitleItalicCheck && m_subtitleItalicCheck->isChecked()) ? QStringLiteral("italic") : QStringLiteral("normal");

    QString style = QStringLiteral(
        "background-color: #111111; "
        "border: 1px solid #333333; "
        "border-radius: 4px; "
        "padding: 12px; "
        "color: %1; "
        "font-weight: %2; "
        "font-style: %3;"
    ).arg(m_subtitleColor.name())
     .arg(weightStr)
     .arg(italicStr);

    m_subtitlePreviewLabel->setStyleSheet(style);
}

void PreferencesDialog::applyDarkTheme()
{
    const QString accent = ThemeManager::instance().accentHex();
    const QString baseSheet = ThemeManager::instance().styleSheet();

    const QString dialogSheet = QStringLiteral(
        "QDialog {"
        "    background-color: #1e1e1e;"
        "    color: #e0e0e0;"
        "}"
        "QTreeWidget {"
        "    background-color: #252525;"
        "    border: 1px solid #383838;"
        "    color: #e0e0e0;"
        "    outline: none;"
        "}"
        "QTreeWidget::item {"
        "    padding: 4px;"
        "    border-radius: 2px;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: %1;"
        "    color: #000000;"
        "}"
        "QGroupBox {"
        "    border: 1px solid #383838;"
        "    border-radius: 4px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    color: #e0e0e0;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 4px;"
        "}"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "    background-color: #2b2b2b;"
        "    border: 1px solid #444444;"
        "    border-radius: 3px;"
        "    color: #ffffff;"
        "    padding: 4px 6px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "    border: 1px solid %1;"
        "}"
        "QTableWidget {"
        "    background-color: #252525;"
        "    border: 1px solid #383838;"
        "    color: #e0e0e0;"
        "    gridline-color: #333333;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: %1;"
        "    color: #000000;"
        "}"
        "QHeaderView::section {"
        "    background-color: #2b2b2b;"
        "    color: #cccccc;"
        "    padding: 4px;"
        "    border: 1px solid #383838;"
        "}"
    ).arg(accent);

    setStyleSheet(baseSheet + QStringLiteral("\n") + dialogSheet);
}

} // namespace outplayer::ui
