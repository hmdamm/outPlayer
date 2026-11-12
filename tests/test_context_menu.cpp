// SPDX-License-Identifier: GPL-3.0-or-later
#include <QAction>
#include <QApplication>
#include <QList>
#include <QMenu>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <QWidget>
#include <memory>

#include "core/CommandId.h"
#include "core/CommandRegistry.h"
#include "engine/IPlaybackEngine.h"
#include "ui/ContextMenuManager.h"

using namespace outplayer;

class MockPlaybackEngine : public engine::IPlaybackEngine {
    Q_OBJECT
public:
    explicit MockPlaybackEngine(QObject* parent = nullptr) : engine::IPlaybackEngine(parent) {}

    bool initialize() override { return true; }
    bool loadFile(const QString&) override { return true; }
    void play() override { m_paused = false; }
    void pause() override { m_paused = true; }
    void togglePause() override { m_paused = !m_paused; }
    void stop() override {}
    void seek(double pos, bool = false) override { m_position = pos; }
    double position() const override { return m_position; }
    double duration() const override { return 120.0; }
    bool isPaused() const override { return m_paused; }

    QList<engine::MediaTrack> tracks(const QString& type = {}) const override {
        if (type == QStringLiteral("sub")) return m_subTracks;
        if (type == QStringLiteral("audio")) return m_audioTracks;
        if (type == QStringLiteral("video")) return m_videoTracks;
        return {};
    }
    bool setTrack(const QString& type, int trackId) override {
        m_lastTrackType = type;
        m_lastTrackId = trackId;
        return true;
    }
    double volume() const override { return 80.0; }
    void setVolume(double) override {}
    bool isMuted() const override { return false; }
    void setMuted(bool) override {}
    double speed() const override { return 1.0; }
    void setSpeed(double) override {}
    QString aspectRatio() const override { return QStringLiteral("16:9"); }
    void setAspectRatio(const QString& ar) override { m_lastAspectRatio = ar; }
    QList<engine::ChapterInfo> chapters() const override { return m_chapters; }
    bool setChapter(int idx) override {
        m_lastChapterIndex = idx;
        return true;
    }

    int brightness() const override { return 0; }
    void setBrightness(int) override {}
    int contrast() const override { return 0; }
    void setContrast(int) override {}
    int saturation() const override { return 0; }
    void setSaturation(int) override {}
    int hue() const override { return 0; }
    void setHue(int) override {}
    int rotation() const override { return 0; }
    void setRotation(int) override {}
    bool deinterlace() const override { return false; }
    void setDeinterlace(bool) override {}
    void resetVideoEqualizer() override {}

    double audioDelay() const override { return 0.0; }
    void setAudioDelay(double) override {}
    double equalizerBand(int) const override { return 0.0; }
    void setEqualizerBand(int, double) override {}
    void resetAudioEqualizer() override {}
    bool isNormalizerEnabled() const override { return false; }
    void setNormalizerEnabled(bool) override {}

    double subtitleDelay() const override { return 0.0; }
    void setSubtitleDelay(double) override {}
    double subtitleScale() const override { return 1.0; }
    void setSubtitleScale(double) override {}
    bool loadSubtitle(const QString&) override { return true; }

    bool captureFrame(const QString&) override { return true; }

    QList<engine::MediaTrack> m_subTracks;
    QList<engine::MediaTrack> m_audioTracks;
    QList<engine::MediaTrack> m_videoTracks;
    QList<engine::ChapterInfo> m_chapters;

    QString m_lastTrackType;
    int m_lastTrackId{-99};
    QString m_lastAspectRatio;
    int m_lastChapterIndex{-1};
    double m_position{0.0};
    bool m_paused{false};
};

class TestContextMenu : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void testTopLevelCategories();
    void testSubmenuStructure();
    void testCommandExecutionViaActions();
    void testCustomDialogSignals();
    void testShortcutAssignments();
    void testDarkPaletteStyleSheet();
    void testDynamicEngineTracksAndChapters();
    void testShowContextMenuSafe();

private:
    static QAction* findActionByText(QMenu* menu, const QString& text);
    static QAction* findActionByCommand(QMenu* menu, core::CommandId id);
};

void TestContextMenu::initTestCase() {
    QVERIFY(qApp != nullptr);
}

void TestContextMenu::cleanup() {
    core::CommandRegistry::instance().clearHandlers();
}

QAction* TestContextMenu::findActionByText(QMenu* menu, const QString& text) {
    if (!menu) return nullptr;
    for (QAction* act : menu->actions()) {
        if (act->text() == text) return act;
        if (act->menu()) {
            if (QAction* child = findActionByText(act->menu(), text)) {
                return child;
            }
        }
    }
    return nullptr;
}

QAction* TestContextMenu::findActionByCommand(QMenu* menu, core::CommandId id) {
    if (!menu) return nullptr;
    for (QAction* act : menu->actions()) {
        if (act->data().isValid() && act->data().canConvert<core::CommandId>()) {
            if (act->data().value<core::CommandId>() == id) {
                return act;
            }
        }
        if (act->menu()) {
            if (QAction* child = findActionByCommand(act->menu(), id)) {
                return child;
            }
        }
    }
    return nullptr;
}

void TestContextMenu::testTopLevelCategories() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    const QStringList expected = {
        QStringLiteral("Open"),
        QStringLiteral("Playback"),
        QStringLiteral("Subtitles"),
        QStringLiteral("Video"),
        QStringLiteral("Audio"),
        QStringLiteral("Window Size"),
        QStringLiteral("Playlist"),
        QStringLiteral("Control Panel"),
        QStringLiteral("Preferences"),
        QStringLiteral("About..."),
        QStringLiteral("Exit")
    };

    QStringList actual;
    for (QAction* act : menu->actions()) {
        if (!act->isSeparator()) {
            actual.append(act->text());
        }
    }

    QCOMPARE(actual, expected);
}

void TestContextMenu::testSubmenuStructure() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    // Verify submenus exist
    QAction* openAct = findActionByText(menu.get(), QStringLiteral("Open"));
    QVERIFY(openAct && openAct->menu());

    QAction* playbackAct = findActionByText(menu.get(), QStringLiteral("Playback"));
    QVERIFY(playbackAct && playbackAct->menu());

    QAction* subAct = findActionByText(menu.get(), QStringLiteral("Subtitles"));
    QVERIFY(subAct && subAct->menu());

    QAction* videoAct = findActionByText(menu.get(), QStringLiteral("Video"));
    QVERIFY(videoAct && videoAct->menu());

    QAction* audioAct = findActionByText(menu.get(), QStringLiteral("Audio"));
    QVERIFY(audioAct && audioAct->menu());

    QAction* winSizeAct = findActionByText(menu.get(), QStringLiteral("Window Size"));
    QVERIFY(winSizeAct && winSizeAct->menu());

    // Direct actions
    QAction* playlistAct = findActionByText(menu.get(), QStringLiteral("Playlist"));
    QVERIFY(playlistAct && !playlistAct->menu());

    QAction* ctrlPanelAct = findActionByText(menu.get(), QStringLiteral("Control Panel"));
    QVERIFY(ctrlPanelAct && !ctrlPanelAct->menu());

    QAction* prefAct = findActionByText(menu.get(), QStringLiteral("Preferences"));
    QVERIFY(prefAct && !prefAct->menu());

    QAction* aboutAct = findActionByText(menu.get(), QStringLiteral("About..."));
    QVERIFY(aboutAct && !aboutAct->menu());

    QAction* exitAct = findActionByText(menu.get(), QStringLiteral("Exit"));
    QVERIFY(exitAct && !exitAct->menu());

    // Check inner actions
    QVERIFY(findActionByText(openAct->menu(), QStringLiteral("Open File(s)...")) != nullptr);
    QVERIFY(findActionByText(openAct->menu(), QStringLiteral("Open Folder...")) != nullptr);
    QVERIFY(findActionByText(openAct->menu(), QStringLiteral("Open URL...")) != nullptr);

    QVERIFY(findActionByText(playbackAct->menu(), QStringLiteral("Play/Pause")) != nullptr);
    QVERIFY(findActionByText(playbackAct->menu(), QStringLiteral("Stop")) != nullptr);
    QVERIFY(findActionByText(playbackAct->menu(), QStringLiteral("Previous")) != nullptr);
    QVERIFY(findActionByText(playbackAct->menu(), QStringLiteral("Next")) != nullptr);

    QVERIFY(findActionByText(subAct->menu(), QStringLiteral("Show/Hide Subtitles")) != nullptr);
    QVERIFY(findActionByText(subAct->menu(), QStringLiteral("Load Subtitles...")) != nullptr);

    QVERIFY(findActionByText(videoAct->menu(), QStringLiteral("Fullscreen")) != nullptr);

    QVERIFY(findActionByText(audioAct->menu(), QStringLiteral("Volume Up")) != nullptr);
    QVERIFY(findActionByText(audioAct->menu(), QStringLiteral("Volume Down")) != nullptr);
    QVERIFY(findActionByText(audioAct->menu(), QStringLiteral("Mute")) != nullptr);
}

void TestContextMenu::testCommandExecutionViaActions() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    auto& registry = core::CommandRegistry::instance();

    bool playPauseExecuted = false;
    registry.registerHandler(core::CommandId::CMD_PLAY_PAUSE, [&](const QVariantMap&) {
        playPauseExecuted = true;
    });

    bool stopExecuted = false;
    registry.registerHandler(core::CommandId::CMD_PLAY_STOP, [&](const QVariantMap&) {
        stopExecuted = true;
    });

    bool volumeUpExecuted = false;
    registry.registerHandler(core::CommandId::CMD_VOLUME_UP, [&](const QVariantMap&) {
        volumeUpExecuted = true;
    });

    bool fullscreenExecuted = false;
    registry.registerHandler(core::CommandId::CMD_WINDOW_FULLSCREEN, [&](const QVariantMap&) {
        fullscreenExecuted = true;
    });

    bool exitExecuted = false;
    registry.registerHandler(core::CommandId::CMD_EXIT, [&](const QVariantMap&) {
        exitExecuted = true;
    });

    QAction* playPauseAct = findActionByCommand(menu.get(), core::CommandId::CMD_PLAY_PAUSE);
    QVERIFY(playPauseAct != nullptr);
    playPauseAct->trigger();
    QVERIFY(playPauseExecuted);

    QAction* stopAct = findActionByCommand(menu.get(), core::CommandId::CMD_PLAY_STOP);
    QVERIFY(stopAct != nullptr);
    stopAct->trigger();
    QVERIFY(stopExecuted);

    QAction* volUpAct = findActionByCommand(menu.get(), core::CommandId::CMD_VOLUME_UP);
    QVERIFY(volUpAct != nullptr);
    volUpAct->trigger();
    QVERIFY(volumeUpExecuted);

    QAction* fsAct = findActionByCommand(menu.get(), core::CommandId::CMD_WINDOW_FULLSCREEN);
    QVERIFY(fsAct != nullptr);
    fsAct->trigger();
    QVERIFY(fullscreenExecuted);

    QAction* exitAct = findActionByCommand(menu.get(), core::CommandId::CMD_EXIT);
    QVERIFY(exitAct != nullptr);
    exitAct->trigger();
    QVERIFY(exitExecuted);
}

void TestContextMenu::testCustomDialogSignals() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    QSignalSpy spyOpenFiles(&mgr, &ui::ContextMenuManager::openFilesRequested);
    QSignalSpy spyOpenFolder(&mgr, &ui::ContextMenuManager::openFolderRequested);
    QSignalSpy spyOpenUrl(&mgr, &ui::ContextMenuManager::openUrlRequested);
    QSignalSpy spyPlaylist(&mgr, &ui::ContextMenuManager::togglePlaylistRequested);
    QSignalSpy spyControlPanel(&mgr, &ui::ContextMenuManager::toggleControlPanelRequested);
    QSignalSpy spyPreferences(&mgr, &ui::ContextMenuManager::preferencesRequested);
    QSignalSpy spyAbout(&mgr, &ui::ContextMenuManager::aboutRequested);

    QAction* openFilesAct = findActionByText(menu.get(), QStringLiteral("Open File(s)..."));
    QVERIFY(openFilesAct != nullptr);
    openFilesAct->trigger();
    QCOMPARE(spyOpenFiles.count(), 1);

    QAction* openFolderAct = findActionByText(menu.get(), QStringLiteral("Open Folder..."));
    QVERIFY(openFolderAct != nullptr);
    openFolderAct->trigger();
    QCOMPARE(spyOpenFolder.count(), 1);

    QAction* openUrlAct = findActionByText(menu.get(), QStringLiteral("Open URL..."));
    QVERIFY(openUrlAct != nullptr);
    openUrlAct->trigger();
    QCOMPARE(spyOpenUrl.count(), 1);

    QAction* playlistAct = findActionByText(menu.get(), QStringLiteral("Playlist"));
    QVERIFY(playlistAct != nullptr);
    playlistAct->trigger();
    QCOMPARE(spyPlaylist.count(), 1);

    QAction* ctrlPanelAct = findActionByText(menu.get(), QStringLiteral("Control Panel"));
    QVERIFY(ctrlPanelAct != nullptr);
    ctrlPanelAct->trigger();
    QCOMPARE(spyControlPanel.count(), 1);

    QAction* prefAct = findActionByText(menu.get(), QStringLiteral("Preferences"));
    QVERIFY(prefAct != nullptr);
    prefAct->trigger();
    QCOMPARE(spyPreferences.count(), 1);

    QAction* aboutAct = findActionByText(menu.get(), QStringLiteral("About..."));
    QVERIFY(aboutAct != nullptr);
    aboutAct->trigger();
    QCOMPARE(spyAbout.count(), 1);
}

void TestContextMenu::testShortcutAssignments() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    QAction* playPauseAct = findActionByCommand(menu.get(), core::CommandId::CMD_PLAY_PAUSE);
    QVERIFY(playPauseAct != nullptr);
    QCOMPARE(playPauseAct->shortcut().toString(), QStringLiteral("Space"));

    QAction* prefAct = findActionByText(menu.get(), QStringLiteral("Preferences"));
    QVERIFY(prefAct != nullptr);
    QCOMPARE(prefAct->shortcut().toString(), QStringLiteral("F5"));

    QAction* playlistAct = findActionByText(menu.get(), QStringLiteral("Playlist"));
    QVERIFY(playlistAct != nullptr);
    QCOMPARE(playlistAct->shortcut().toString(), QStringLiteral("F6"));

    QAction* ctrlPanelAct = findActionByText(menu.get(), QStringLiteral("Control Panel"));
    QVERIFY(ctrlPanelAct != nullptr);
    QCOMPARE(ctrlPanelAct->shortcut().toString(), QStringLiteral("F7"));

    QAction* aboutAct = findActionByText(menu.get(), QStringLiteral("About..."));
    QVERIFY(aboutAct != nullptr);
    QCOMPARE(aboutAct->shortcut().toString(), QStringLiteral("F1"));
}

void TestContextMenu::testDarkPaletteStyleSheet() {
    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr));
    QVERIFY(menu != nullptr);

    const QString ss = menu->styleSheet();
    QVERIFY(ss.contains(QStringLiteral("#252525")));
    QVERIFY(ss.contains(QStringLiteral("#383838")));
    QVERIFY(ss.contains(QStringLiteral("#ffaa00")));
}

void TestContextMenu::testDynamicEngineTracksAndChapters() {
    MockPlaybackEngine engine;
    engine.m_subTracks = {
        { .id = 1, .type = QStringLiteral("sub"), .title = QStringLiteral("English"), .selected = true },
        { .id = 2, .type = QStringLiteral("sub"), .title = QStringLiteral("Spanish"), .selected = false }
    };
    engine.m_audioTracks = {
        { .id = 1, .type = QStringLiteral("audio"), .title = QStringLiteral("Stereo"), .selected = true }
    };
    engine.m_chapters = {
        { .index = 0, .title = QStringLiteral("Intro"), .time = 0.0 },
        { .index = 1, .title = QStringLiteral("Part 1"), .time = 60.0 }
    };

    ui::ContextMenuManager mgr;
    std::unique_ptr<QMenu> menu(mgr.buildMainMenu(nullptr, &engine));
    QVERIFY(menu != nullptr);

    QAction* spanishSub = findActionByText(menu.get(), QStringLiteral("English"));
    QVERIFY(spanishSub != nullptr);
    QVERIFY(spanishSub->isChecked());

    QAction* secondSub = findActionByText(menu.get(), QStringLiteral("Spanish"));
    QVERIFY(secondSub != nullptr);
    QVERIFY(!secondSub->isChecked());
    secondSub->trigger();
    QCOMPARE(engine.m_lastTrackType, QStringLiteral("sub"));
    QCOMPARE(engine.m_lastTrackId, 2);

    QAction* ch2 = findActionByText(menu.get(), QStringLiteral("Part 1"));
    QVERIFY(ch2 != nullptr);
    ch2->trigger();
    QCOMPARE(engine.m_lastChapterIndex, 1);
}

void TestContextMenu::testShowContextMenuSafe() {
    ui::ContextMenuManager mgr;
    // Calling showContextMenu should not crash even with null parent
    mgr.showContextMenu(QPoint(100, 100), nullptr, nullptr);
}

QTEST_MAIN(TestContextMenu)
#include "test_context_menu.moc"
