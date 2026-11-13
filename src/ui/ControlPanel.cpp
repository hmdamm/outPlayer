// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/ControlPanel.h"
#include "engine/IPlaybackEngine.h"
#include "ui/TimeFormatter.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace outplayer::ui {

ControlPanel::ControlPanel(engine::IPlaybackEngine* engine, QWidget* parent)
    : QWidget(parent, Qt::Tool)
    , m_engine(engine)
{
    setWindowTitle(tr("Control Panel"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMinimumSize(400, 340);
    resize(440, 380);

    setupUi();
    setupStyle();

    if (m_engine) {
        connect(m_engine, &engine::IPlaybackEngine::videoEqualizerChanged,
                this, &ControlPanel::updateVideoFromEngine);
        connect(m_engine, &engine::IPlaybackEngine::audioEqualizerChanged,
                this, &ControlPanel::updateAudioFromEngine);
        connect(m_engine, &engine::IPlaybackEngine::subtitleSettingsChanged,
                this, &ControlPanel::updateSubtitlesFromEngine);
        connect(m_engine, &engine::IPlaybackEngine::tracksChanged,
                this, &ControlPanel::updateSubtitleTracksFromEngine);
        connect(m_engine, &engine::IPlaybackEngine::speedChanged,
                this, &ControlPanel::updatePlaybackFromEngine);
        connect(m_engine, &engine::IPlaybackEngine::stateChanged,
                this, &ControlPanel::updatePlaybackFromEngine);
    }

    updateFromEngine();
}

QSize ControlPanel::sizeHint() const {
    return QSize(440, 380);
}

int ControlPanel::currentTabIndex() const {
    return m_tabWidget ? m_tabWidget->currentIndex() : -1;
}

void ControlPanel::setCurrentTabIndex(int index) {
    if (m_tabWidget && index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

QSlider* ControlPanel::equalizerSlider(int bandIndex) const {
    if (bandIndex >= 0 && bandIndex < m_eqSliders.size()) {
        return m_eqSliders[bandIndex];
    }
    return nullptr;
}

void ControlPanel::closeEvent(QCloseEvent* event) {
    emit closeRequested();
    QWidget::closeEvent(event);
}

void ControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName(QStringLiteral("controlPanelTabWidget"));
    m_tabWidget->setDocumentMode(true);

    m_closeBtn = new QPushButton(QStringLiteral("✕"), m_tabWidget);
    m_closeBtn->setObjectName(QStringLiteral("controlPanelCloseBtn"));
    m_closeBtn->setToolTip(tr("Close"));
    m_closeBtn->setFixedSize(26, 24);
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::close);
    m_tabWidget->setCornerWidget(m_closeBtn, Qt::TopRightCorner);

    setupVideoTab();
    setupAudioTab();
    setupSubtitlesTab();
    setupPlaybackTab();

    mainLayout->addWidget(m_tabWidget);
}

void ControlPanel::setupVideoTab() {
    auto* videoWidget = new QWidget(m_tabWidget);
    auto* videoLayout = new QVBoxLayout(videoWidget);
    videoLayout->setContentsMargins(12, 12, 12, 12);
    videoLayout->setSpacing(8);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    auto createSliderRow = [this, videoWidget, grid](int row, const QString& labelText,
                                                     QSlider*& outSlider, QLabel*& outValLabel,
                                                     QPushButton*& outResetBtn, auto setter) {
        auto* label = new QLabel(labelText, videoWidget);
        label->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
        label->setFixedWidth(75);

        auto* slider = new QSlider(Qt::Horizontal, videoWidget);
        slider->setRange(-100, 100);
        slider->setValue(0);
        outSlider = slider;

        auto* valLabel = new QLabel(QStringLiteral("0"), videoWidget);
        valLabel->setFixedWidth(35);
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valLabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-family: monospace;"));
        outValLabel = valLabel;

        auto* resetBtn = new QPushButton(tr("Reset"), videoWidget);
        resetBtn->setFixedWidth(50);
        outResetBtn = resetBtn;

        grid->addWidget(label, row, 0);
        grid->addWidget(slider, row, 1);
        grid->addWidget(valLabel, row, 2);
        grid->addWidget(resetBtn, row, 3);

        connect(slider, &QSlider::valueChanged, this, [this, valLabel, setter](int val) {
            valLabel->setText(QString::number(val));
            if (!m_updatingFromEngine && m_engine) {
                (m_engine->*setter)(val);
            }
        });

        connect(resetBtn, &QPushButton::clicked, this, [slider]() {
            slider->setValue(0);
        });
    };

    createSliderRow(0, tr("Brightness:"), m_brightnessSlider, m_brightnessLabel,
                    m_brightnessResetBtn, &engine::IPlaybackEngine::setBrightness);
    createSliderRow(1, tr("Contrast:"), m_contrastSlider, m_contrastLabel,
                    m_contrastResetBtn, &engine::IPlaybackEngine::setContrast);
    createSliderRow(2, tr("Saturation:"), m_saturationSlider, m_saturationLabel,
                    m_saturationResetBtn, &engine::IPlaybackEngine::setSaturation);
    createSliderRow(3, tr("Hue:"), m_hueSlider, m_hueLabel,
                    m_hueResetBtn, &engine::IPlaybackEngine::setHue);

    videoLayout->addLayout(grid);

    // Transform row: Deinterlace & Rotation
    auto* transformLayout = new QHBoxLayout();
    transformLayout->setSpacing(12);

    m_deinterlaceCheckBox = new QCheckBox(tr("Deinterlacing"), videoWidget);
    connect(m_deinterlaceCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setDeinterlace(checked);
        }
    });
    transformLayout->addWidget(m_deinterlaceCheckBox);

    transformLayout->addStretch(1);

    auto* rotLabel = new QLabel(tr("Rotation:"), videoWidget);
    rotLabel->setStyleSheet(QStringLiteral("color: #cccccc;"));
    transformLayout->addWidget(rotLabel);

    m_rotationComboBox = new QComboBox(videoWidget);
    m_rotationComboBox->addItem(QStringLiteral("0°"), 0);
    m_rotationComboBox->addItem(QStringLiteral("90°"), 90);
    m_rotationComboBox->addItem(QStringLiteral("180°"), 180);
    m_rotationComboBox->addItem(QStringLiteral("270°"), 270);
    connect(m_rotationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (!m_updatingFromEngine && m_engine) {
            const int degrees = m_rotationComboBox->itemData(index).toInt();
            m_engine->setRotation(degrees);
        }
    });
    transformLayout->addWidget(m_rotationComboBox);

    videoLayout->addLayout(transformLayout);

    videoLayout->addSpacing(4);

    // Global Reset
    m_resetAllVideoBtn = new QPushButton(tr("Reset All Video Adjustments"), videoWidget);
    m_resetAllVideoBtn->setObjectName(QStringLiteral("resetAllVideoBtn"));
    connect(m_resetAllVideoBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine) {
            m_engine->resetVideoEqualizer();
            m_engine->setRotation(0);
            m_engine->setDeinterlace(false);
        }
        m_brightnessSlider->setValue(0);
        m_contrastSlider->setValue(0);
        m_saturationSlider->setValue(0);
        m_hueSlider->setValue(0);
        m_rotationComboBox->setCurrentIndex(0);
        m_deinterlaceCheckBox->setChecked(false);
    });
    videoLayout->addWidget(m_resetAllVideoBtn);

    videoLayout->addStretch(1);
    m_tabWidget->addTab(videoWidget, tr("Video"));
}

void ControlPanel::setupAudioTab() {
    auto* audioWidget = new QWidget(m_tabWidget);
    auto* audioLayout = new QVBoxLayout(audioWidget);
    audioLayout->setContentsMargins(12, 10, 12, 10);
    audioLayout->setSpacing(6);

    // 10-Band Equalizer
    auto* eqBox = new QGroupBox(tr("10-Band Graphic Equalizer (-12dB .. +12dB)"), audioWidget);
    auto* eqLayout = new QHBoxLayout(eqBox);
    eqLayout->setContentsMargins(6, 12, 6, 6);
    eqLayout->setSpacing(4);

    const QStringList frequencies = {
        QStringLiteral("31Hz"),  QStringLiteral("62Hz"),  QStringLiteral("125Hz"),
        QStringLiteral("250Hz"), QStringLiteral("500Hz"), QStringLiteral("1kHz"),
        QStringLiteral("2kHz"),  QStringLiteral("4kHz"),  QStringLiteral("8kHz"),
        QStringLiteral("16kHz")
    };

    m_eqSliders.clear();
    for (int i = 0; i < 10; ++i) {
        auto* colWidget = new QWidget(eqBox);
        auto* colLayout = new QVBoxLayout(colWidget);
        colLayout->setContentsMargins(0, 0, 0, 0);
        colLayout->setSpacing(2);

        auto* valLabel = new QLabel(QStringLiteral("0.0"), colWidget);
        valLabel->setAlignment(Qt::AlignCenter);
        valLabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-size: 10px; font-family: monospace;"));

        auto* slider = new QSlider(Qt::Vertical, colWidget);
        slider->setRange(-24, 24); // step of 0.5dB => -12.0dB .. +12.0dB
        slider->setValue(0);
        slider->setFixedHeight(110);

        auto* freqLabel = new QLabel(frequencies[i], colWidget);
        freqLabel->setAlignment(Qt::AlignCenter);
        freqLabel->setStyleSheet(QStringLiteral("color: #888888; font-size: 9px;"));

        colLayout->addWidget(valLabel);
        colLayout->addWidget(slider, 1, Qt::AlignHCenter);
        colLayout->addWidget(freqLabel);

        m_eqSliders.append(slider);
        eqLayout->addWidget(colWidget);

        connect(slider, &QSlider::valueChanged, this, [this, i, valLabel](int val) {
            const double gainDb = val * 0.5;
            valLabel->setText(QString::asprintf("%+.1f", gainDb));
            if (!m_updatingFromEngine && m_engine) {
                m_engine->setEqualizerBand(i, gainDb);
            }
        });
    }

    audioLayout->addWidget(eqBox);

    // Audio Buttons row: Reset EQ
    auto* eqBtnLayout = new QHBoxLayout();
    m_resetEqBtn = new QPushButton(tr("Reset EQ"), audioWidget);
    m_resetEqBtn->setFixedWidth(90);
    connect(m_resetEqBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine) {
            m_engine->resetAudioEqualizer();
        }
        for (auto* slider : m_eqSliders) {
            slider->setValue(0);
        }
    });
    eqBtnLayout->addWidget(m_resetEqBtn);
    eqBtnLayout->addStretch(1);
    audioLayout->addLayout(eqBtnLayout);

    // Normalizer and Audio Delay
    auto* bottomAudioLayout = new QHBoxLayout();
    bottomAudioLayout->setSpacing(12);

    m_normalizerCheckBox = new QCheckBox(tr("Normalize Audio (Dialogue Boost)"), audioWidget);
    connect(m_normalizerCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setNormalizerEnabled(checked);
        }
    });
    bottomAudioLayout->addWidget(m_normalizerCheckBox);

    bottomAudioLayout->addStretch(1);

    auto* delayLabel = new QLabel(tr("Audio Delay:"), audioWidget);
    delayLabel->setStyleSheet(QStringLiteral("color: #cccccc;"));
    bottomAudioLayout->addWidget(delayLabel);

    m_audioDelaySpinBox = new QDoubleSpinBox(audioWidget);
    m_audioDelaySpinBox->setRange(-10.0, 10.0);
    m_audioDelaySpinBox->setSingleStep(0.05);
    m_audioDelaySpinBox->setDecimals(2);
    m_audioDelaySpinBox->setSuffix(QStringLiteral(" s"));
    m_audioDelaySpinBox->setValue(0.0);
    connect(m_audioDelaySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setAudioDelay(val);
        }
    });
    bottomAudioLayout->addWidget(m_audioDelaySpinBox);

    m_audioDelayResetBtn = new QPushButton(tr("Reset"), audioWidget);
    m_audioDelayResetBtn->setFixedWidth(50);
    connect(m_audioDelayResetBtn, &QPushButton::clicked, this, [this]() {
        m_audioDelaySpinBox->setValue(0.0);
    });
    bottomAudioLayout->addWidget(m_audioDelayResetBtn);

    audioLayout->addLayout(bottomAudioLayout);
    audioLayout->addStretch(1);

    m_tabWidget->addTab(audioWidget, tr("Audio / EQ"));
}

void ControlPanel::setupSubtitlesTab() {
    auto* subWidget = new QWidget(m_tabWidget);
    auto* subLayout = new QVBoxLayout(subWidget);
    subLayout->setContentsMargins(12, 12, 12, 12);
    subLayout->setSpacing(10);

    // Subtitle track selection row
    auto* trackLayout = new QHBoxLayout();
    auto* trackLabel = new QLabel(tr("Subtitle Track:"), subWidget);
    trackLabel->setFixedWidth(90);
    trackLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    trackLayout->addWidget(trackLabel);

    m_subtitleTrackComboBox = new QComboBox(subWidget);
    m_subtitleTrackComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_subtitleTrackComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (!m_updatingFromEngine && m_engine && index >= 0) {
            const int trackId = m_subtitleTrackComboBox->itemData(index).toInt();
            m_engine->setTrack(QStringLiteral("sub"), trackId);
        }
    });
    trackLayout->addWidget(m_subtitleTrackComboBox);

    m_loadSubtitleBtn = new QPushButton(tr("Load Subtitle..."), subWidget);
    connect(m_loadSubtitleBtn, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("Load Subtitle"),
            QString(),
            tr("Subtitles (*.srt *.ass *.ssa *.vtt *.sub *.idx);;All Files (*)")
        );
        if (!filePath.isEmpty() && m_engine) {
            m_engine->loadSubtitle(filePath);
        }
    });
    trackLayout->addWidget(m_loadSubtitleBtn);

    subLayout->addLayout(trackLayout);

    // Font and Size row
    auto* fontLayout = new QHBoxLayout();
    auto* fontLabel = new QLabel(tr("Font Family:"), subWidget);
    fontLabel->setFixedWidth(90);
    fontLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    fontLayout->addWidget(fontLabel);

    m_subtitleFontCombo = new QFontComboBox(subWidget);
    m_subtitleFontCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_subtitleFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setSubtitleFont(font.family());
        }
    });
    fontLayout->addWidget(m_subtitleFontCombo);

    auto* sizeLabel = new QLabel(tr("Size:"), subWidget);
    sizeLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    fontLayout->addWidget(sizeLabel);

    m_subtitleFontSizeSpinBox = new QSpinBox(subWidget);
    m_subtitleFontSizeSpinBox->setRange(10, 96);
    m_subtitleFontSizeSpinBox->setValue(24);
    m_subtitleFontSizeSpinBox->setSuffix(QStringLiteral(" pt"));
    connect(m_subtitleFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setSubtitleFontSize(val);
        }
    });
    fontLayout->addWidget(m_subtitleFontSizeSpinBox);

    subLayout->addLayout(fontLayout);

    // Sync Offset row
    auto* delayLayout = new QHBoxLayout();
    auto* delayLabel = new QLabel(tr("Sync Offset:"), subWidget);
    delayLabel->setFixedWidth(90);
    delayLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    delayLayout->addWidget(delayLabel);

    m_subtitleDelaySpinBox = new QDoubleSpinBox(subWidget);
    m_subtitleDelaySpinBox->setRange(-60.0, 60.0);
    m_subtitleDelaySpinBox->setSingleStep(0.1);
    m_subtitleDelaySpinBox->setDecimals(2);
    m_subtitleDelaySpinBox->setSuffix(QStringLiteral(" s"));
    m_subtitleDelaySpinBox->setValue(0.0);
    connect(m_subtitleDelaySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setSubtitleDelay(val);
        }
    });
    delayLayout->addWidget(m_subtitleDelaySpinBox);

    m_subtitleDelayMinusBtn = new QPushButton(QStringLiteral("-0.5s"), subWidget);
    m_subtitleDelayMinusBtn->setFixedWidth(50);
    connect(m_subtitleDelayMinusBtn, &QPushButton::clicked, this, [this]() {
        m_subtitleDelaySpinBox->setValue(m_subtitleDelaySpinBox->value() - 0.5);
    });
    delayLayout->addWidget(m_subtitleDelayMinusBtn);

    m_subtitleDelayPlusBtn = new QPushButton(QStringLiteral("+0.5s"), subWidget);
    m_subtitleDelayPlusBtn->setFixedWidth(50);
    connect(m_subtitleDelayPlusBtn, &QPushButton::clicked, this, [this]() {
        m_subtitleDelaySpinBox->setValue(m_subtitleDelaySpinBox->value() + 0.5);
    });
    delayLayout->addWidget(m_subtitleDelayPlusBtn);

    m_subtitleDelayResetBtn = new QPushButton(tr("Reset"), subWidget);
    m_subtitleDelayResetBtn->setFixedWidth(50);
    connect(m_subtitleDelayResetBtn, &QPushButton::clicked, this, [this]() {
        m_subtitleDelaySpinBox->setValue(0.0);
    });
    delayLayout->addWidget(m_subtitleDelayResetBtn);

    subLayout->addLayout(delayLayout);

    // Font Scale row
    auto* scaleLayout = new QHBoxLayout();
    auto* scaleLabel = new QLabel(tr("Font Scale:"), subWidget);
    scaleLabel->setFixedWidth(90);
    scaleLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    scaleLayout->addWidget(scaleLabel);

    m_subtitleScaleSlider = new QSlider(Qt::Horizontal, subWidget);
    m_subtitleScaleSlider->setRange(50, 200);
    m_subtitleScaleSlider->setValue(100);
    scaleLayout->addWidget(m_subtitleScaleSlider);

    m_subtitleScaleLabel = new QLabel(QStringLiteral("100%"), subWidget);
    m_subtitleScaleLabel->setFixedWidth(45);
    m_subtitleScaleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_subtitleScaleLabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-family: monospace;"));
    scaleLayout->addWidget(m_subtitleScaleLabel);

    m_subtitleScaleResetBtn = new QPushButton(tr("Reset"), subWidget);
    m_subtitleScaleResetBtn->setFixedWidth(50);
    connect(m_subtitleScaleResetBtn, &QPushButton::clicked, this, [this]() {
        m_subtitleScaleSlider->setValue(100);
    });
    scaleLayout->addWidget(m_subtitleScaleResetBtn);

    connect(m_subtitleScaleSlider, &QSlider::valueChanged, this, [this](int val) {
        m_subtitleScaleLabel->setText(QString::number(val) + QStringLiteral("%"));
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setSubtitleScale(val / 100.0);
        }
    });

    subLayout->addLayout(scaleLayout);
    subLayout->addStretch(1);

    m_tabWidget->addTab(subWidget, tr("Subtitles"));
}

void ControlPanel::setupPlaybackTab() {
    auto* playWidget = new QWidget(m_tabWidget);
    auto* playLayout = new QVBoxLayout(playWidget);
    playLayout->setContentsMargins(12, 12, 12, 12);
    playLayout->setSpacing(10);

    // Speed Slider row
    auto* speedLayout = new QHBoxLayout();
    auto* speedTextLabel = new QLabel(tr("Speed:"), playWidget);
    speedTextLabel->setFixedWidth(60);
    speedTextLabel->setStyleSheet(QStringLiteral("color: #cccccc; font-weight: 500;"));
    speedLayout->addWidget(speedTextLabel);

    m_speedSlider = new QSlider(Qt::Horizontal, playWidget);
    m_speedSlider->setRange(2, 120); // 0.2x to 12.0x (value / 10)
    m_speedSlider->setValue(10);
    speedLayout->addWidget(m_speedSlider);

    m_speedLabel = new QLabel(QStringLiteral("1.0x"), playWidget);
    m_speedLabel->setFixedWidth(45);
    m_speedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_speedLabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-family: monospace;"));
    speedLayout->addWidget(m_speedLabel);

    m_speedResetBtn = new QPushButton(QStringLiteral("1.0x"), playWidget);
    m_speedResetBtn->setFixedWidth(50);
    connect(m_speedResetBtn, &QPushButton::clicked, this, [this]() {
        m_speedSlider->setValue(10);
    });
    speedLayout->addWidget(m_speedResetBtn);

    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int val) {
        const double spd = val / 10.0;
        m_speedLabel->setText(QString::asprintf("%.1fx", spd));
        if (!m_updatingFromEngine && m_engine) {
            m_engine->setSpeed(spd);
        }
    });

    playLayout->addLayout(speedLayout);

    // Quick Speed Buttons
    auto* quickSpeedLayout = new QHBoxLayout();
    quickSpeedLayout->setSpacing(6);
    quickSpeedLayout->addSpacing(65);

    auto addSpeedBtn = [this, playWidget, quickSpeedLayout](const QString& text, int sliderVal) {
        auto* btn = new QPushButton(text, playWidget);
        btn->setFixedWidth(50);
        connect(btn, &QPushButton::clicked, this, [this, sliderVal]() {
            m_speedSlider->setValue(sliderVal);
        });
        quickSpeedLayout->addWidget(btn);
    };

    addSpeedBtn(QStringLiteral("0.5x"), 5);
    addSpeedBtn(QStringLiteral("1.0x"), 10);
    addSpeedBtn(QStringLiteral("1.5x"), 15);
    addSpeedBtn(QStringLiteral("2.0x"), 20);
    quickSpeedLayout->addStretch(1);

    playLayout->addLayout(quickSpeedLayout);

    // Play/Pause transport control
    auto* transportLayout = new QHBoxLayout();
    m_playPauseBtn = new QPushButton(tr("▶ Play / ❚❚ Pause"), playWidget);
    m_playPauseBtn->setObjectName(QStringLiteral("playPauseBtn"));
    connect(m_playPauseBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine) {
            m_engine->togglePause();
        }
    });
    transportLayout->addWidget(m_playPauseBtn);
    playLayout->addLayout(transportLayout);

    // A-B Repeat Controller
    auto* abBox = new QGroupBox(tr("A-B Repeat"), playWidget);
    auto* abLayout = new QHBoxLayout(abBox);
    abLayout->setContentsMargins(8, 8, 8, 8);
    abLayout->setSpacing(6);

    m_setPointABtn = new QPushButton(tr("Set Point A ["), abBox);
    connect(m_setPointABtn, &QPushButton::clicked, this, [this]() {
        if (m_engine) {
            m_pointA = m_engine->position();
            m_pointALabel->setText(TimeFormatter::formatTime(m_pointA, true));
        }
    });
    abLayout->addWidget(m_setPointABtn);

    m_pointALabel = new QLabel(QStringLiteral("--:--:--"), abBox);
    m_pointALabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-family: monospace;"));
    abLayout->addWidget(m_pointALabel);

    m_setPointBBtn = new QPushButton(tr("Set Point B ]"), abBox);
    connect(m_setPointBBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine) {
            m_pointB = m_engine->position();
            m_pointBLabel->setText(TimeFormatter::formatTime(m_pointB, true));
        }
    });
    abLayout->addWidget(m_setPointBBtn);

    m_pointBLabel = new QLabel(QStringLiteral("--:--:--"), abBox);
    m_pointBLabel->setStyleSheet(QStringLiteral("color: #ffaa00; font-family: monospace;"));
    abLayout->addWidget(m_pointBLabel);

    m_clearABBtn = new QPushButton(tr("Clear"), abBox);
    connect(m_clearABBtn, &QPushButton::clicked, this, [this]() {
        m_pointA = -1.0;
        m_pointB = -1.0;
        m_pointALabel->setText(QStringLiteral("--:--:--"));
        m_pointBLabel->setText(QStringLiteral("--:--:--"));
    });
    abLayout->addWidget(m_clearABBtn);

    playLayout->addWidget(abBox);
    playLayout->addStretch(1);

    m_tabWidget->addTab(playWidget, tr("Playback"));
}

void ControlPanel::setupStyle() {
    setStyleSheet(QStringLiteral(
        "QWidget {"
        "    background-color: #1b1b1b;"
        "    color: #dddddd;"
        "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
        "    font-size: 11px;"
        "}"
        "QTabWidget::pane {"
        "    background-color: #1b1b1b;"
        "    border-top: 1px solid #303030;"
        "    border-left: none;"
        "    border-right: none;"
        "    border-bottom: none;"
        "}"
        "QTabBar {"
        "    background-color: #252525;"
        "}"
        "QTabBar::tab {"
        "    background-color: #252525;"
        "    color: #aaaaaa;"
        "    padding: 6px 14px;"
        "    border: none;"
        "    font-size: 11px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #1b1b1b;"
        "    color: #ffffff;"
        "    border-bottom: 2px solid #ffaa00;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: #2a2a2a;"
        "    color: #ffffff;"
        "}"
        "QGroupBox {"
        "    border: 1px solid #303030;"
        "    border-radius: 4px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    color: #888888;"
        "    font-weight: 500;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 8px;"
        "    padding: 0 4px;"
        "}"
        "QPushButton {"
        "    background-color: #252525;"
        "    color: #cccccc;"
        "    border: 1px solid #383838;"
        "    border-radius: 3px;"
        "    padding: 3px 6px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #323232;"
        "    color: #ffffff;"
        "    border-color: #4a4a4a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #404040;"
        "}"
        "QPushButton#controlPanelCloseBtn {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #888888;"
        "    font-size: 13px;"
        "    padding: 2px 4px;"
        "}"
        "QPushButton#controlPanelCloseBtn:hover {"
        "    color: #ffffff;"
        "    background-color: #e81123;"
        "}"
        "QSlider::groove:horizontal {"
        "    height: 4px;"
        "    background-color: #383838;"
        "    border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background-color: #e09b1f;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background-color: #ffffff;"
        "    width: 12px;"
        "    margin: -4px 0;"
        "    border-radius: 6px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background-color: #ffaa00;"
        "}"
        "QSlider::groove:vertical {"
        "    width: 4px;"
        "    background-color: #383838;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:vertical {"
        "    background-color: #ffffff;"
        "    height: 12px;"
        "    margin: 0 -4px;"
        "    border-radius: 6px;"
        "}"
        "QSlider::handle:vertical:hover {"
        "    background-color: #ffaa00;"
        "}"
        "QComboBox, QDoubleSpinBox {"
        "    background-color: #181818;"
        "    color: #ffffff;"
        "    border: 1px solid #333333;"
        "    border-radius: 3px;"
        "    padding: 3px 6px;"
        "}"
        "QComboBox:focus, QDoubleSpinBox:focus {"
        "    border-color: #ffaa00;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 18px;"
        "}"
        "QCheckBox {"
        "    color: #cccccc;"
        "    spacing: 5px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 14px;"
        "    height: 14px;"
        "    border: 1px solid #444444;"
        "    border-radius: 2px;"
        "    background-color: #252525;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #ffaa00;"
        "    border-color: #ffaa00;"
        "}"
    ));
}

void ControlPanel::updateFromEngine() {
    updateVideoFromEngine();
    updateAudioFromEngine();
    updateSubtitlesFromEngine();
    updateSubtitleTracksFromEngine();
    updatePlaybackFromEngine();
}

void ControlPanel::updateVideoFromEngine() {
    if (!m_engine) return;
    m_updatingFromEngine = true;

    const int b = m_engine->brightness();
    m_brightnessSlider->setValue(b);
    m_brightnessLabel->setText(QString::number(b));

    const int c = m_engine->contrast();
    m_contrastSlider->setValue(c);
    m_contrastLabel->setText(QString::number(c));

    const int s = m_engine->saturation();
    m_saturationSlider->setValue(s);
    m_saturationLabel->setText(QString::number(s));

    const int h = m_engine->hue();
    m_hueSlider->setValue(h);
    m_hueLabel->setText(QString::number(h));

    const int rot = m_engine->rotation();
    int rotIdx = 0;
    if (rot == 90) rotIdx = 1;
    else if (rot == 180) rotIdx = 2;
    else if (rot == 270) rotIdx = 3;
    m_rotationComboBox->setCurrentIndex(rotIdx);

    m_deinterlaceCheckBox->setChecked(m_engine->deinterlace());

    m_updatingFromEngine = false;
}

void ControlPanel::updateAudioFromEngine() {
    if (!m_engine) return;
    m_updatingFromEngine = true;

    for (int i = 0; i < m_eqSliders.size() && i < 10; ++i) {
        const double gainDb = m_engine->equalizerBand(i);
        const int sliderVal = std::clamp(static_cast<int>(std::round(gainDb * 2.0)), -24, 24);
        m_eqSliders[i]->setValue(sliderVal);
    }

    m_normalizerCheckBox->setChecked(m_engine->isNormalizerEnabled());
    m_audioDelaySpinBox->setValue(m_engine->audioDelay());

    m_updatingFromEngine = false;
}

void ControlPanel::updateSubtitlesFromEngine() {
    if (!m_engine) return;
    m_updatingFromEngine = true;

    m_subtitleDelaySpinBox->setValue(m_engine->subtitleDelay());

    const double scale = m_engine->subtitleScale();
    const int scalePercent = std::clamp(static_cast<int>(std::round(scale * 100.0)), 50, 200);
    m_subtitleScaleSlider->setValue(scalePercent);
    m_subtitleScaleLabel->setText(QString::number(scalePercent) + QStringLiteral("%"));

    if (m_subtitleFontCombo) {
        m_subtitleFontCombo->setCurrentFont(QFont(m_engine->subtitleFont()));
    }
    if (m_subtitleFontSizeSpinBox) {
        m_subtitleFontSizeSpinBox->setValue(m_engine->subtitleFontSize());
    }

    m_updatingFromEngine = false;
}

void ControlPanel::updateSubtitleTracksFromEngine() {
    if (!m_engine) return;
    m_updatingFromEngine = true;

    m_subtitleTrackComboBox->clear();
    const auto subTracks = m_engine->tracks(QStringLiteral("sub"));
    int selectedIdx = -1;

    for (int i = 0; i < subTracks.size(); ++i) {
        const auto& trk = subTracks[i];
        QString label = trk.title;
        if (label.isEmpty()) {
            label = trk.language.isEmpty() ? trk.codec : trk.language;
        }
        if (label.isEmpty()) {
            label = trk.type.isEmpty() ? QStringLiteral("Track %1").arg(trk.id) : trk.type;
        }
        m_subtitleTrackComboBox->addItem(label, trk.id);
        if (trk.selected) {
            selectedIdx = i;
        }
    }

    if (selectedIdx >= 0) {
        m_subtitleTrackComboBox->setCurrentIndex(selectedIdx);
    }

    m_updatingFromEngine = false;
}

void ControlPanel::updatePlaybackFromEngine() {
    if (!m_engine) return;
    m_updatingFromEngine = true;

    const double spd = m_engine->speed();
    const int spdVal = std::clamp(static_cast<int>(std::round(spd * 10.0)), 2, 120);
    m_speedSlider->setValue(spdVal);
    m_speedLabel->setText(QString::asprintf("%.1fx", spd));

    m_playPauseBtn->setText(m_engine->isPaused() ? tr("▶ Play") : tr("❚❚ Pause"));

    m_updatingFromEngine = false;
}

} // namespace outplayer::ui
