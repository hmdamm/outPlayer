// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/ControlBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "engine/IPlaybackEngine.h"
#include "ui/TimeFormatter.h"
#include "ui/TimelineSlider.h"

namespace outplayer::ui {

ControlBar::ControlBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(56);
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    applyStyle();
}

void ControlBar::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 2, 8, 4);
    mainLayout->setSpacing(2);

    // --- Top Tier: Timeline Slider + Mute + Volume (~18px) ---
    auto* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);
    topLayout->setAlignment(Qt::AlignVCenter);

    // Integrated expanding timeline slider
    m_timelineSlider = new TimelineSlider(this);
    connect(m_timelineSlider, &TimelineSlider::seekRequested, this, [this](double sec) {
        if (m_engine) {
            m_engine->seek(sec);
        }
    });
    topLayout->addWidget(m_timelineSlider, 1);

    // Mute button (20x20)
    m_muteBtn = new QPushButton(QStringLiteral("VOL"), this);
    m_muteBtn->setFixedSize(20, 20);
    m_muteBtn->setToolTip(QStringLiteral("Mute / Unmute (M)"));
    connect(m_muteBtn, &QPushButton::clicked, this, &ControlBar::onMuteClicked);
    topLayout->addWidget(m_muteBtn);

    // Volume slider (70px width, 0 to 100)
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(70);
    m_volumeSlider->setFixedHeight(16);
    m_volumeSlider->setToolTip(QStringLiteral("Volume"));
    connect(m_volumeSlider, &QSlider::valueChanged, this, &ControlBar::onVolumeSliderValueChanged);
    topLayout->addWidget(m_volumeSlider);

    mainLayout->addLayout(topLayout);

    // --- Bottom Tier: Play/Pause, Stop, Prev, Next, Open, Time, Badges, Right Utilities (~38px) ---
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(4);
    bottomLayout->setAlignment(Qt::AlignVCenter);

    // Play/Pause button (28x28)
    m_playPauseBtn = new QPushButton(QStringLiteral("▶"), this);
    m_playPauseBtn->setFixedSize(28, 28);
    m_playPauseBtn->setToolTip(QStringLiteral("Play (Space)"));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &ControlBar::onPlayPauseClicked);
    bottomLayout->addWidget(m_playPauseBtn);

    // Stop button (24x24)
    m_stopBtn = new QPushButton(QStringLiteral("■"), this);
    m_stopBtn->setFixedSize(24, 24);
    m_stopBtn->setToolTip(QStringLiteral("Stop (Ctrl+Space)"));
    connect(m_stopBtn, &QPushButton::clicked, this, &ControlBar::onStopClicked);
    bottomLayout->addWidget(m_stopBtn);

    // Previous track / chapter button (24x24)
    m_prevBtn = new QPushButton(QStringLiteral("|◀"), this);
    m_prevBtn->setFixedSize(24, 24);
    m_prevBtn->setToolTip(QStringLiteral("Previous (PageUp)"));
    connect(m_prevBtn, &QPushButton::clicked, this, &ControlBar::onPrevClicked);
    bottomLayout->addWidget(m_prevBtn);

    // Next track / chapter button (24x24)
    m_nextBtn = new QPushButton(QStringLiteral("▶|"), this);
    m_nextBtn->setFixedSize(24, 24);
    m_nextBtn->setToolTip(QStringLiteral("Next (PageDown)"));
    connect(m_nextBtn, &QPushButton::clicked, this, &ControlBar::onNextClicked);
    bottomLayout->addWidget(m_nextBtn);

    // Open File button (24x24, ⏏)
    m_openFileBtn = new QPushButton(QStringLiteral("⏏"), this);
    m_openFileBtn->setFixedSize(24, 24);
    m_openFileBtn->setToolTip(QStringLiteral("Open File... (Ctrl+O)"));
    connect(m_openFileBtn, &QPushButton::clicked, this, &ControlBar::onOpenFileClicked);
    bottomLayout->addWidget(m_openFileBtn);

    // Elapsed / Total duration label
    m_timeLabel = new QLabel(QStringLiteral("00:00:00 / 00:00:00"), this);
    m_timeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    bottomLayout->addWidget(m_timeLabel);

    // H/W badge
    m_hwBadge = new QLabel(QStringLiteral("H/W"), this);
    m_hwBadge->setObjectName(QStringLiteral("hwBadge"));
    m_hwBadge->setStyleSheet(QStringLiteral(
        "color: #ffaa00; background-color: #332800; border: 1px solid #665200; "
        "border-radius: 2px; padding: 1px 4px; font-size: 9px; font-weight: bold;"
    ));
    m_hwBadge->setVisible(false);
    bottomLayout->addWidget(m_hwBadge);

    // Video codec badge
    m_videoCodecBadge = new QLabel(this);
    m_videoCodecBadge->setObjectName(QStringLiteral("videoCodecBadge"));
    m_videoCodecBadge->setStyleSheet(QStringLiteral(
        "color: #cccccc; background-color: #1a1a1a; border: 1px solid #333333; "
        "border-radius: 2px; padding: 1px 4px; font-size: 9px; font-weight: bold;"
    ));
    m_videoCodecBadge->setVisible(false);
    bottomLayout->addWidget(m_videoCodecBadge);

    // Audio codec badge
    m_audioCodecBadge = new QLabel(this);
    m_audioCodecBadge->setObjectName(QStringLiteral("audioCodecBadge"));
    m_audioCodecBadge->setStyleSheet(QStringLiteral(
        "color: #cccccc; background-color: #1a1a1a; border: 1px solid #333333; "
        "border-radius: 2px; padding: 1px 4px; font-size: 9px; font-weight: bold;"
    ));
    m_audioCodecBadge->setVisible(false);
    bottomLayout->addWidget(m_audioCodecBadge);

    // Audio channels badge
    m_audioChannelsBadge = new QLabel(this);
    m_audioChannelsBadge->setObjectName(QStringLiteral("audioChannelsBadge"));
    m_audioChannelsBadge->setStyleSheet(QStringLiteral(
        "color: #cccccc; background-color: #1a1a1a; border: 1px solid #333333; "
        "border-radius: 2px; padding: 1px 4px; font-size: 9px; font-weight: bold;"
    ));
    m_audioChannelsBadge->setVisible(false);
    bottomLayout->addWidget(m_audioChannelsBadge);

    // Stretch between stream badges and right utility buttons
    bottomLayout->addStretch(1);

    // Subtitle toggle button (24x24)
    m_subBtn = new QPushButton(QStringLiteral("SUB"), this);
    m_subBtn->setFixedSize(24, 24);
    m_subBtn->setToolTip(QStringLiteral("Subtitles (Alt+H)"));
    connect(m_subBtn, &QPushButton::clicked, this, &ControlBar::onSubtitleClicked);
    bottomLayout->addWidget(m_subBtn);

    // Control panel toggle button (24x24)
    m_controlPanelBtn = new QPushButton(QStringLiteral("CP"), this);
    m_controlPanelBtn->setFixedSize(24, 24);
    m_controlPanelBtn->setToolTip(QStringLiteral("Control Panel (F7)"));
    connect(m_controlPanelBtn, &QPushButton::clicked, this, &ControlBar::onControlPanelClicked);
    bottomLayout->addWidget(m_controlPanelBtn);

    // Settings button (24x24)
    m_settingsBtn = new QPushButton(QStringLiteral("⚙"), this);
    m_settingsBtn->setFixedSize(24, 24);
    m_settingsBtn->setToolTip(QStringLiteral("Preferences (F5)"));
    connect(m_settingsBtn, &QPushButton::clicked, this, &ControlBar::onSettingsClicked);
    bottomLayout->addWidget(m_settingsBtn);

    // Playlist toggle button (24x24)
    m_playlistBtn = new QPushButton(QStringLiteral("☰"), this);
    m_playlistBtn->setFixedSize(24, 24);
    m_playlistBtn->setToolTip(QStringLiteral("Playlist (F6)"));
    connect(m_playlistBtn, &QPushButton::clicked, this, &ControlBar::onPlaylistClicked);
    bottomLayout->addWidget(m_playlistBtn);

    mainLayout->addLayout(bottomLayout);
}

void ControlBar::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "outplayer--ui--ControlBar, ControlBar {"
        "    background-color: #252525;"
        "    border-top: 1px solid #2e2e2e;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #ffffff;"
        "    border: none;"
        "    border-radius: 3px;"
        "    font-size: 11px;"
        "    font-weight: 500;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #323232;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #404040;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "    font-family: \"JetBrains Mono\", \"Roboto Mono\", \"Fira Code\", monospace;"
        "    font-size: 10pt;"
        "    padding-left: 4px;"
        "    padding-right: 4px;"
        "}"
        "QSlider::groove:horizontal {"
        "    height: 4px;"
        "    background: #383838;"
        "    border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: #ffaa00;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: #ffffff;"
        "    width: 10px;"
        "    margin-top: -3px;"
        "    margin-bottom: -3px;"
        "    border-radius: 5px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: #ffaa00;"
        "}"
    ));
}

void ControlBar::setPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;
    m_playPauseBtn->setText(isPlaying ? QStringLiteral("❚❚") : QStringLiteral("▶"));
    m_playPauseBtn->setToolTip(isPlaying ? QStringLiteral("Pause (Space)") : QStringLiteral("Play (Space)"));
}

void ControlBar::setTime(double currentSeconds, double totalSeconds)
{
    m_currentPosition = std::max(0.0, currentSeconds);
    m_duration = std::max(0.0, totalSeconds);
    m_timeLabel->setText(TimeFormatter::formatTimePair(m_currentPosition, m_duration));
    if (m_timelineSlider) {
        m_timelineSlider->setDuration(m_duration);
        m_timelineSlider->setPosition(m_currentPosition);
    }
}

void ControlBar::setVolume(double volumePercent)
{
    m_volume = std::clamp(volumePercent, 0.0, 100.0);
    if (m_volumeSlider) {
        const QSignalBlocker blocker(m_volumeSlider);
        m_volumeSlider->setValue(static_cast<int>(std::round(m_volume)));
    }
}

void ControlBar::setMuted(bool isMuted)
{
    m_isMuted = isMuted;
    if (m_muteBtn) {
        m_muteBtn->setText(isMuted ? QStringLiteral("MUT") : QStringLiteral("VOL"));
        m_muteBtn->setToolTip(isMuted ? QStringLiteral("Unmute (M)") : QStringLiteral("Mute (M)"));
        m_muteBtn->setStyleSheet(isMuted ? QStringLiteral("color: #ffaa00;") : QString());
    }
}

void ControlBar::setVideoInfo(bool hwDec, const QString& videoCodec, const QString& audioCodec, const QString& channels)
{
    m_isHwDecoding = hwDec;
    m_videoCodec = videoCodec;
    m_audioCodec = audioCodec;
    m_audioChannels = channels;

    if (m_hwBadge) {
        m_hwBadge->setVisible(hwDec);
    }
    if (m_videoCodecBadge) {
        m_videoCodecBadge->setText(videoCodec.toUpper());
        m_videoCodecBadge->setVisible(!videoCodec.trimmed().isEmpty());
    }
    if (m_audioCodecBadge) {
        m_audioCodecBadge->setText(audioCodec.toUpper());
        m_audioCodecBadge->setVisible(!audioCodec.trimmed().isEmpty());
    }
    if (m_audioChannelsBadge) {
        m_audioChannelsBadge->setText(channels.toUpper());
        m_audioChannelsBadge->setVisible(!channels.trimmed().isEmpty());
    }
}

void ControlBar::setPlaybackEngine(engine::IPlaybackEngine* engine)
{
    if (m_engine) {
        disconnect(m_engine, nullptr, this, nullptr);
    }

    m_engine = engine;
    if (!m_engine) {
        return;
    }

    connect(m_engine, &engine::IPlaybackEngine::stateChanged, this, [this](bool paused) {
        setPlaying(!paused);
    });
    connect(m_engine, &engine::IPlaybackEngine::positionChanged, this, [this](double sec) {
        m_currentPosition = sec;
        setTime(m_currentPosition, m_duration);
    });
    connect(m_engine, &engine::IPlaybackEngine::durationChanged, this, [this](double sec) {
        m_duration = sec;
        setTime(m_currentPosition, m_duration);
    });
    connect(m_engine, &engine::IPlaybackEngine::volumeChanged, this, &ControlBar::setVolume);
    connect(m_engine, &engine::IPlaybackEngine::muteChanged, this, &ControlBar::setMuted);
    connect(m_engine, &engine::IPlaybackEngine::chaptersChanged, this, [this]() {
        if (m_engine && m_timelineSlider) {
            m_timelineSlider->setChapters(m_engine->chapters());
        }
    });

    // Initialize UI from current engine state
    setPlaying(!m_engine->isPaused());
    m_currentPosition = m_engine->position();
    m_duration = m_engine->duration();
    setTime(m_currentPosition, m_duration);
    setVolume(m_engine->volume());
    setMuted(m_engine->isMuted());
    if (m_timelineSlider) {
        m_timelineSlider->setChapters(m_engine->chapters());
    }
}

TimelineSlider* ControlBar::timelineSlider() const noexcept
{
    return m_timelineSlider;
}

QLabel* ControlBar::timeLabel() const noexcept
{
    return m_timeLabel;
}

QPushButton* ControlBar::playPauseButton() const noexcept
{
    return m_playPauseBtn;
}

QPushButton* ControlBar::stopButton() const noexcept
{
    return m_stopBtn;
}

QPushButton* ControlBar::prevButton() const noexcept
{
    return m_prevBtn;
}

QPushButton* ControlBar::nextButton() const noexcept
{
    return m_nextBtn;
}

QPushButton* ControlBar::openFileButton() const noexcept
{
    return m_openFileBtn;
}

QPushButton* ControlBar::muteButton() const noexcept
{
    return m_muteBtn;
}

QSlider* ControlBar::volumeSlider() const noexcept
{
    return m_volumeSlider;
}

QPushButton* ControlBar::subtitleButton() const noexcept
{
    return m_subBtn;
}

QPushButton* ControlBar::playlistButton() const noexcept
{
    return m_playlistBtn;
}

QPushButton* ControlBar::controlPanelButton() const noexcept
{
    return m_controlPanelBtn;
}

QPushButton* ControlBar::settingsButton() const noexcept
{
    return m_settingsBtn;
}

QLabel* ControlBar::hwBadge() const noexcept
{
    return m_hwBadge;
}

QLabel* ControlBar::videoCodecBadge() const noexcept
{
    return m_videoCodecBadge;
}

QLabel* ControlBar::audioCodecBadge() const noexcept
{
    return m_audioCodecBadge;
}

QLabel* ControlBar::audioChannelsBadge() const noexcept
{
    return m_audioChannelsBadge;
}

bool ControlBar::isPlaying() const noexcept
{
    return m_isPlaying;
}

bool ControlBar::isMuted() const noexcept
{
    return m_isMuted;
}

double ControlBar::volume() const noexcept
{
    return m_volume;
}

double ControlBar::currentPosition() const noexcept
{
    return m_currentPosition;
}

double ControlBar::duration() const noexcept
{
    return m_duration;
}

bool ControlBar::isHwDecoding() const noexcept
{
    return m_isHwDecoding;
}

QString ControlBar::videoCodec() const noexcept
{
    return m_videoCodec;
}

QString ControlBar::audioCodec() const noexcept
{
    return m_audioCodec;
}

QString ControlBar::audioChannels() const noexcept
{
    return m_audioChannels;
}

QSize ControlBar::sizeHint() const
{
    return QSize(600, 56);
}

QSize ControlBar::minimumSizeHint() const
{
    return QSize(450, 56);
}

void ControlBar::onPlayPauseClicked()
{
    emit playPauseClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAY_PAUSE);
}

void ControlBar::onStopClicked()
{
    emit stopClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAY_STOP);
}

void ControlBar::onPrevClicked()
{
    emit prevClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_PREV);
}

void ControlBar::onNextClicked()
{
    emit nextClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST_NEXT);
}

void ControlBar::onOpenFileClicked()
{
    emit openFileClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_OPEN_FILE);
}

void ControlBar::onMuteClicked()
{
    emit muteToggled();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_VOLUME_MUTE);
}

void ControlBar::onVolumeSliderValueChanged(int value)
{
    m_volume = static_cast<double>(value);
    emit volumeChanged(m_volume);
    if (m_engine) {
        m_engine->setVolume(m_volume);
    }
}

void ControlBar::onSubtitleClicked()
{
    emit subtitleToggled();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_SUB_TOGGLE_VISIBILITY);
}

void ControlBar::onPlaylistClicked()
{
    emit playlistToggled();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST);
}

void ControlBar::onControlPanelClicked()
{
    emit controlPanelToggled();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_CONTROL_PANEL);
}

void ControlBar::onSettingsClicked()
{
    emit preferencesClicked();
    core::CommandRegistry::instance().execute(core::CommandId::CMD_PREFERENCES);
}

} // namespace outplayer::ui
