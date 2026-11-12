// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/ContextMenuManager.h"

#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QMenu>
#include <QWidget>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "engine/IPlaybackEngine.h"

namespace outplayer::ui {

namespace {

constexpr char kDarkMenuStyleSheet[] =
    "QMenu {"
    "    background-color: #252525;"
    "    color: #e0e0e0;"
    "    border: 1px solid #383838;"
    "    padding: 4px 0px;"
    "}"
    "QMenu::item {"
    "    background-color: transparent;"
    "    padding: 6px 24px 6px 20px;"
    "    color: #e0e0e0;"
    "}"
    "QMenu::item:selected {"
    "    background-color: #383838;"
    "    color: #ffaa00;"
    "}"
    "QMenu::item:disabled {"
    "    color: #666666;"
    "}"
    "QMenu::separator {"
    "    height: 1px;"
    "    background: #383838;"
    "    margin: 4px 8px;"
    "}";

} // namespace

ContextMenuManager::ContextMenuManager(QObject* parent)
    : QObject(parent)
{
}

void ContextMenuManager::applyStyleSheet(QMenu* menu)
{
    if (menu) {
        menu->setStyleSheet(QString::fromLatin1(kDarkMenuStyleSheet));
    }
}

QAction* ContextMenuManager::addCommandAction(QMenu* menu, const QString& title, core::CommandId cmdId)
{
    if (!menu) {
        return nullptr;
    }

    QAction* action = menu->addAction(title);
    action->setData(QVariant::fromValue(cmdId));

    const auto* info = core::CommandRegistry::instance().getInfo(cmdId);
    if (info && !info->defaultKey.isEmpty()) {
        QString seqStr = info->defaultKey;
        if (!info->defaultModifiers.isEmpty() && info->defaultModifiers != QStringLiteral("None")) {
            seqStr = info->defaultModifiers + QStringLiteral("+") + info->defaultKey;
        }
        action->setShortcut(QKeySequence(seqStr));
        action->setShortcutVisibleInContextMenu(true);
    }

    connect(action, &QAction::triggered, this, [cmdId]() {
        core::CommandRegistry::instance().execute(cmdId);
    });

    return action;
}

QMenu* ContextMenuManager::buildMainMenu(QWidget* parentWidget, engine::IPlaybackEngine* engine)
{
    auto* rootMenu = new QMenu(parentWidget);
    applyStyleSheet(rootMenu);

    // 1. Open
    QMenu* openMenu = rootMenu->addMenu(QStringLiteral("Open"));
    applyStyleSheet(openMenu);

    QAction* openFilesAct = openMenu->addAction(QStringLiteral("Open File(s)..."));
    openFilesAct->setData(QVariant::fromValue(core::CommandId::CMD_OPEN_FILE));
    openFilesAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
    openFilesAct->setShortcutVisibleInContextMenu(true);
    connect(openFilesAct, &QAction::triggered, this, [this]() {
        emit openFilesRequested();
        core::CommandRegistry::instance().execute(core::CommandId::CMD_OPEN_FILE);
        core::CommandRegistry::instance().execute(core::CommandId::CMD_OPEN_FILE_STD);
    });

    QAction* openFolderAct = addCommandAction(openMenu, QStringLiteral("Open Folder..."), core::CommandId::CMD_OPEN_FOLDER);
    connect(openFolderAct, &QAction::triggered, this, &ContextMenuManager::openFolderRequested);

    QAction* openUrlAct = addCommandAction(openMenu, QStringLiteral("Open URL..."), core::CommandId::CMD_OPEN_URL);
    connect(openUrlAct, &QAction::triggered, this, &ContextMenuManager::openUrlRequested);

    openMenu->addSeparator();
    addCommandAction(openMenu, QStringLiteral("Open DVD"), core::CommandId::CMD_OPEN_DVD);
    addCommandAction(openMenu, QStringLiteral("Open Blu-ray"), core::CommandId::CMD_OPEN_BLURAY);
    addCommandAction(openMenu, QStringLiteral("Open Screen Capture"), core::CommandId::CMD_OPEN_SCREEN_CAPTURE);
    addCommandAction(openMenu, QStringLiteral("Open Webcam"), core::CommandId::CMD_OPEN_WEBCAM);
    openMenu->addSeparator();
    addCommandAction(openMenu, QStringLiteral("Close Playback"), core::CommandId::CMD_CLOSE_PLAYBACK);

    rootMenu->addSeparator();

    // 2. Playback
    QMenu* playbackMenu = rootMenu->addMenu(QStringLiteral("Playback"));
    applyStyleSheet(playbackMenu);
    addCommandAction(playbackMenu, QStringLiteral("Play/Pause"), core::CommandId::CMD_PLAY_PAUSE);
    addCommandAction(playbackMenu, QStringLiteral("Stop"), core::CommandId::CMD_PLAY_STOP);
    playbackMenu->addSeparator();
    addCommandAction(playbackMenu, QStringLiteral("Previous"), core::CommandId::CMD_PLAYLIST_PREV);
    addCommandAction(playbackMenu, QStringLiteral("Next"), core::CommandId::CMD_PLAYLIST_NEXT);
    playbackMenu->addSeparator();

    QMenu* jumpMenu = playbackMenu->addMenu(QStringLiteral("Jump"));
    applyStyleSheet(jumpMenu);
    addCommandAction(jumpMenu, QStringLiteral("5s Backward"), core::CommandId::CMD_SEEK_BACKWARD_SHORT);
    addCommandAction(jumpMenu, QStringLiteral("5s Forward"), core::CommandId::CMD_SEEK_FORWARD_SHORT);
    addCommandAction(jumpMenu, QStringLiteral("30s Backward"), core::CommandId::CMD_SEEK_BACKWARD_MEDIUM);
    addCommandAction(jumpMenu, QStringLiteral("30s Forward"), core::CommandId::CMD_SEEK_FORWARD_MEDIUM);
    addCommandAction(jumpMenu, QStringLiteral("1m Backward"), core::CommandId::CMD_SEEK_BACKWARD_1MIN);
    addCommandAction(jumpMenu, QStringLiteral("1m Forward"), core::CommandId::CMD_SEEK_FORWARD_1MIN);
    addCommandAction(jumpMenu, QStringLiteral("5m Backward"), core::CommandId::CMD_SEEK_BACKWARD_5MIN);
    addCommandAction(jumpMenu, QStringLiteral("5m Forward"), core::CommandId::CMD_SEEK_FORWARD_5MIN);
    jumpMenu->addSeparator();
    addCommandAction(jumpMenu, QStringLiteral("Previous Frame"), core::CommandId::CMD_FRAME_PREV);
    addCommandAction(jumpMenu, QStringLiteral("Next Frame"), core::CommandId::CMD_FRAME_NEXT);
    addCommandAction(jumpMenu, QStringLiteral("Beginning"), core::CommandId::CMD_SEEK_BEGINNING);
    addCommandAction(jumpMenu, QStringLiteral("End"), core::CommandId::CMD_SEEK_END);

    QMenu* speedMenu = playbackMenu->addMenu(QStringLiteral("Speed"));
    applyStyleSheet(speedMenu);
    addCommandAction(speedMenu, QStringLiteral("Normal (1.0x)"), core::CommandId::CMD_SPEED_NORMAL);
    addCommandAction(speedMenu, QStringLiteral("Faster"), core::CommandId::CMD_SPEED_UP);
    addCommandAction(speedMenu, QStringLiteral("Slower"), core::CommandId::CMD_SPEED_DOWN);

    if (engine) {
        const auto chs = engine->chapters();
        if (!chs.isEmpty()) {
            QMenu* chaptersMenu = playbackMenu->addMenu(QStringLiteral("Chapters"));
            applyStyleSheet(chaptersMenu);
            for (const auto& ch : chs) {
                QString title = ch.title.isEmpty()
                    ? QStringLiteral("Chapter %1").arg(ch.index + 1)
                    : ch.title;
                QAction* chAct = chaptersMenu->addAction(title);
                connect(chAct, &QAction::triggered, this, [engine, idx = ch.index]() {
                    engine->setChapter(idx);
                });
            }
        }
    }

    // 3. Subtitles
    QMenu* subMenu = rootMenu->addMenu(QStringLiteral("Subtitles"));
    applyStyleSheet(subMenu);
    addCommandAction(subMenu, QStringLiteral("Show/Hide Subtitles"), core::CommandId::CMD_SUB_TOGGLE_VISIBILITY);
    addCommandAction(subMenu, QStringLiteral("Load Subtitles..."), core::CommandId::CMD_SUB_LOAD);
    subMenu->addSeparator();

    if (engine) {
        QMenu* subTracksMenu = subMenu->addMenu(QStringLiteral("Subtitle Tracks"));
        applyStyleSheet(subTracksMenu);
        const auto subTracks = engine->tracks(QStringLiteral("sub"));
        if (subTracks.isEmpty()) {
            QAction* noneAct = subTracksMenu->addAction(QStringLiteral("No Subtitles"));
            noneAct->setEnabled(false);
        } else {
            auto* subGroup = new QActionGroup(subTracksMenu);
            QAction* disableAct = subTracksMenu->addAction(QStringLiteral("Disable Subtitles"));
            disableAct->setCheckable(true);
            subGroup->addAction(disableAct);

            bool anySelected = false;
            for (const auto& trk : subTracks) {
                QString label = trk.title.isEmpty()
                    ? (trk.language.isEmpty() ? QStringLiteral("Track %1").arg(trk.id) : trk.language)
                    : trk.title;
                QAction* tAct = subTracksMenu->addAction(label);
                tAct->setCheckable(true);
                tAct->setChecked(trk.selected);
                if (trk.selected) {
                    anySelected = true;
                }
                subGroup->addAction(tAct);
                connect(tAct, &QAction::triggered, this, [engine, id = trk.id]() {
                    engine->setTrack(QStringLiteral("sub"), id);
                });
            }

            disableAct->setChecked(!anySelected);
            connect(disableAct, &QAction::triggered, this, [engine]() {
                engine->setTrack(QStringLiteral("sub"), -1);
            });
        }
    }

    QMenu* subSyncMenu = subMenu->addMenu(QStringLiteral("Subtitle Sync"));
    applyStyleSheet(subSyncMenu);
    addCommandAction(subSyncMenu, QStringLiteral("Faster (0.5s)"), core::CommandId::CMD_SUB_SYNC_FASTER);
    addCommandAction(subSyncMenu, QStringLiteral("Slower (0.5s)"), core::CommandId::CMD_SUB_SYNC_SLOWER);
    addCommandAction(subSyncMenu, QStringLiteral("Reset Sync"), core::CommandId::CMD_SUB_SYNC_RESET);

    QMenu* subSizeMenu = subMenu->addMenu(QStringLiteral("Subtitle Size"));
    applyStyleSheet(subSizeMenu);
    addCommandAction(subSizeMenu, QStringLiteral("Increase Size"), core::CommandId::CMD_SUB_SIZE_UP);
    addCommandAction(subSizeMenu, QStringLiteral("Decrease Size"), core::CommandId::CMD_SUB_SIZE_DOWN);
    addCommandAction(subSizeMenu, QStringLiteral("Reset Size"), core::CommandId::CMD_SUB_SIZE_RESET);

    // 4. Video
    QMenu* videoMenu = rootMenu->addMenu(QStringLiteral("Video"));
    applyStyleSheet(videoMenu);
    addCommandAction(videoMenu, QStringLiteral("Fullscreen"), core::CommandId::CMD_WINDOW_FULLSCREEN);

    QMenu* arMenu = videoMenu->addMenu(QStringLiteral("Aspect Ratio"));
    applyStyleSheet(arMenu);
    QAction* arAuto = addCommandAction(arMenu, QStringLiteral("Auto / Default"), core::CommandId::CMD_ASPECT_RATIO_PRESET);
    QAction* ar16_9 = addCommandAction(arMenu, QStringLiteral("16:9"), core::CommandId::CMD_ASPECT_RATIO_3);
    QAction* ar4_3 = addCommandAction(arMenu, QStringLiteral("4:3"), core::CommandId::CMD_ASPECT_RATIO_2);
    QAction* ar185 = addCommandAction(arMenu, QStringLiteral("1.85:1"), core::CommandId::CMD_ASPECT_RATIO_4);
    QAction* ar235 = addCommandAction(arMenu, QStringLiteral("2.35:1"), core::CommandId::CMD_ASPECT_RATIO_5);
    addCommandAction(arMenu, QStringLiteral("Cycle Aspect Ratio"), core::CommandId::CMD_ASPECT_RATIO_CYCLE);

    if (engine) {
        connect(arAuto, &QAction::triggered, this, [engine]() { engine->setAspectRatio(QStringLiteral("auto")); });
        connect(ar16_9, &QAction::triggered, this, [engine]() { engine->setAspectRatio(QStringLiteral("16:9")); });
        connect(ar4_3, &QAction::triggered, this, [engine]() { engine->setAspectRatio(QStringLiteral("4:3")); });
        connect(ar185, &QAction::triggered, this, [engine]() { engine->setAspectRatio(QStringLiteral("1.85:1")); });
        connect(ar235, &QAction::triggered, this, [engine]() { engine->setAspectRatio(QStringLiteral("2.35:1")); });

        const auto videoTracks = engine->tracks(QStringLiteral("video"));
        if (!videoTracks.isEmpty()) {
            QMenu* vTracksMenu = videoMenu->addMenu(QStringLiteral("Video Tracks"));
            applyStyleSheet(vTracksMenu);
            auto* vGroup = new QActionGroup(vTracksMenu);
            for (const auto& trk : videoTracks) {
                QString label = trk.title.isEmpty()
                    ? QStringLiteral("Video Track %1").arg(trk.id)
                    : trk.title;
                QAction* vAct = vTracksMenu->addAction(label);
                vAct->setCheckable(true);
                vAct->setChecked(trk.selected);
                vGroup->addAction(vAct);
                connect(vAct, &QAction::triggered, this, [engine, id = trk.id]() {
                    engine->setTrack(QStringLiteral("video"), id);
                });
            }
        }
    }

    addCommandAction(videoMenu, QStringLiteral("Deinterlace"), core::CommandId::CMD_VIDEO_CYCLE_DEINTERLACE);
    addCommandAction(videoMenu, QStringLiteral("Rotate 90°"), core::CommandId::CMD_VIDEO_ROTATE_90);
    videoMenu->addSeparator();
    addCommandAction(videoMenu, QStringLiteral("Capture Frame"), core::CommandId::CMD_CAPTURE_SAVE_FRAME);
    addCommandAction(videoMenu, QStringLiteral("Copy Frame"), core::CommandId::CMD_CAPTURE_COPY_FRAME);

    // 5. Audio
    QMenu* audioMenu = rootMenu->addMenu(QStringLiteral("Audio"));
    applyStyleSheet(audioMenu);
    addCommandAction(audioMenu, QStringLiteral("Volume Up"), core::CommandId::CMD_VOLUME_UP);
    addCommandAction(audioMenu, QStringLiteral("Volume Down"), core::CommandId::CMD_VOLUME_DOWN);
    addCommandAction(audioMenu, QStringLiteral("Mute"), core::CommandId::CMD_VOLUME_MUTE);
    audioMenu->addSeparator();

    if (engine) {
        QMenu* aTracksMenu = audioMenu->addMenu(QStringLiteral("Audio Tracks"));
        applyStyleSheet(aTracksMenu);
        const auto audioTracks = engine->tracks(QStringLiteral("audio"));
        if (audioTracks.isEmpty()) {
            QAction* noneAct = aTracksMenu->addAction(QStringLiteral("No Audio Tracks"));
            noneAct->setEnabled(false);
        } else {
            auto* aGroup = new QActionGroup(aTracksMenu);
            for (const auto& trk : audioTracks) {
                QString label = trk.title.isEmpty()
                    ? (trk.language.isEmpty() ? QStringLiteral("Track %1").arg(trk.id) : trk.language)
                    : trk.title;
                QAction* aAct = aTracksMenu->addAction(label);
                aAct->setCheckable(true);
                aAct->setChecked(trk.selected);
                aGroup->addAction(aAct);
                connect(aAct, &QAction::triggered, this, [engine, id = trk.id]() {
                    engine->setTrack(QStringLiteral("audio"), id);
                });
            }
        }
    }

    QMenu* aSyncMenu = audioMenu->addMenu(QStringLiteral("Audio Sync"));
    applyStyleSheet(aSyncMenu);
    addCommandAction(aSyncMenu, QStringLiteral("Faster (0.05s)"), core::CommandId::CMD_AUDIO_SYNC_FASTER);
    addCommandAction(aSyncMenu, QStringLiteral("Slower (0.05s)"), core::CommandId::CMD_AUDIO_SYNC_SLOWER);
    addCommandAction(aSyncMenu, QStringLiteral("Reset Sync"), core::CommandId::CMD_AUDIO_SYNC_RESET);

    addCommandAction(audioMenu, QStringLiteral("Equalizer"), core::CommandId::CMD_AUDIO_EQ_TOGGLE);
    addCommandAction(audioMenu, QStringLiteral("Normalize"), core::CommandId::CMD_AUDIO_NORMALIZE);
    addCommandAction(audioMenu, QStringLiteral("Voice Removal"), core::CommandId::CMD_AUDIO_VOICE_REMOVE);

    rootMenu->addSeparator();

    // 6. Window Size
    QMenu* winSizeMenu = rootMenu->addMenu(QStringLiteral("Window Size"));
    applyStyleSheet(winSizeMenu);
    addCommandAction(winSizeMenu, QStringLiteral("Half Size (0.5x)"), core::CommandId::CMD_WINDOW_HALF);
    addCommandAction(winSizeMenu, QStringLiteral("Normal Size (1.0x)"), core::CommandId::CMD_WINDOW_NORMAL);
    addCommandAction(winSizeMenu, QStringLiteral("1.5x Size"), core::CommandId::CMD_WINDOW_1_5X);
    addCommandAction(winSizeMenu, QStringLiteral("Double Size (2.0x)"), core::CommandId::CMD_WINDOW_DOUBLE);
    addCommandAction(winSizeMenu, QStringLiteral("Fit to Screen"), core::CommandId::CMD_WINDOW_FIT_SCREEN);
    addCommandAction(winSizeMenu, QStringLiteral("Maximize"), core::CommandId::CMD_WINDOW_MAXIMUM);
    addCommandAction(winSizeMenu, QStringLiteral("Fullscreen"), core::CommandId::CMD_WINDOW_FULLSCREEN);

    rootMenu->addSeparator();

    // 7. Playlist
    QAction* playlistAct = addCommandAction(rootMenu, QStringLiteral("Playlist"), core::CommandId::CMD_PLAYLIST_TOGGLE);
    connect(playlistAct, &QAction::triggered, this, [this]() {
        emit togglePlaylistRequested();
        core::CommandRegistry::instance().execute(core::CommandId::CMD_PLAYLIST);
    });

    // 8. Control Panel
    QAction* ctrlPanelAct = addCommandAction(rootMenu, QStringLiteral("Control Panel"), core::CommandId::CMD_CONTROL_PANEL);
    connect(ctrlPanelAct, &QAction::triggered, this, &ContextMenuManager::toggleControlPanelRequested);

    rootMenu->addSeparator();

    // 9. Preferences
    QAction* prefAct = addCommandAction(rootMenu, QStringLiteral("Preferences"), core::CommandId::CMD_PREFERENCES);
    connect(prefAct, &QAction::triggered, this, &ContextMenuManager::preferencesRequested);

    // 10. About...
    QAction* aboutAct = addCommandAction(rootMenu, QStringLiteral("About..."), core::CommandId::CMD_ABOUT);
    connect(aboutAct, &QAction::triggered, this, &ContextMenuManager::aboutRequested);

    rootMenu->addSeparator();

    // 11. Exit
    addCommandAction(rootMenu, QStringLiteral("Exit"), core::CommandId::CMD_EXIT);

    return rootMenu;
}

void ContextMenuManager::showContextMenu(const QPoint& globalPos, QWidget* parentWidget, engine::IPlaybackEngine* engine)
{
    QMenu* menu = buildMainMenu(parentWidget, engine);
    if (!menu) {
        return;
    }
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(globalPos);
}

} // namespace outplayer::ui
