// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <algorithm>
#include <cmath>

namespace outplayer::ui {

class TimeFormatter {
public:
    static QString formatTime(double seconds, bool showHours = false)
    {
        if (!std::isfinite(seconds) || seconds < 0.0) {
            seconds = 0.0;
        }

        const auto totalSeconds = static_cast<qint64>(std::floor(seconds));
        const qint64 h = totalSeconds / 3600;
        const qint64 m = (totalSeconds % 3600) / 60;
        const qint64 s = totalSeconds % 60;

        if (showHours || h > 0) {
            return QStringLiteral("%1:%2:%3")
                .arg(h, 2, 10, QChar('0'))
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0'));
        }

        return QStringLiteral("%1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }

    static QString formatTimePair(double currentSeconds, double totalSeconds)
    {
        const bool showHours = (totalSeconds >= 3600.0 || currentSeconds >= 3600.0);
        return QStringLiteral("%1 / %2")
            .arg(formatTime(currentSeconds, showHours), formatTime(totalSeconds, showHours));
    }
};

} // namespace outplayer::ui
