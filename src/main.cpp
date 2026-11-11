// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MainWindow.h"
#include "engine/MpvEngine.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("outPlayer"));
    app.setApplicationDisplayName(QStringLiteral("outPlayer"));
    app.setOrganizationName(QStringLiteral("outPlayer"));
    app.setOrganizationDomain(QStringLiteral("org.outplayer.outPlayer"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("outPlayer - Modern Linux Media Player"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testRunOption(
        QStringLiteral("test-run"),
        QStringLiteral("Run a brief test playback and exit cleanly."));
    parser.addOption(testRunOption);

    parser.addPositionalArgument(QStringLiteral("media"), QStringLiteral("Media file or URL to open."));

    parser.process(app);

    outplayer::ui::MainWindow window;
    window.show();

    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        window.openMedia(positionalArgs.first());
    } else if (parser.isSet(testRunOption)) {
        window.openMedia(QStringLiteral("lavfi://nullsrc=s=640x360:d=1"));
    }

    if (parser.isSet(testRunOption)) {
        if (window.engine()) {
            QObject::connect(window.engine(), &outplayer::engine::IPlaybackEngine::playbackFinished,
                             &app, &QCoreApplication::quit);
        }
        // Fallback timer ensuring prompt non-hanging exit under headless test environments
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
