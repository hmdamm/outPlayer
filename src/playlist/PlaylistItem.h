#pragma once

#include <QString>
#include <QMetaType>

namespace outplayer::playlist {

struct PlaylistItem {
    QString filePath;
    QString title;
    double duration = 0.0;

    bool operator==(const PlaylistItem& other) const = default;
};

} // namespace outplayer::playlist

Q_DECLARE_METATYPE(outplayer::playlist::PlaylistItem)
