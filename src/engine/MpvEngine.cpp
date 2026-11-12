// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/MpvEngine.h"

#include <QMetaObject>

#include <algorithm>
#include <cmath>
#include <clocale>
#include <cstring>

namespace outplayer::engine {

namespace {

QList<MediaTrack> parseTrackList(const mpv_node* node)
{
    QList<MediaTrack> result;
    if (!node || node->format != MPV_FORMAT_NODE_ARRAY || !node->u.list) {
        return result;
    }

    const mpv_node_list* list = node->u.list;
    result.reserve(list->num);

    for (int i = 0; i < list->num; ++i) {
        const mpv_node& item = list->values[i];
        if (item.format != MPV_FORMAT_NODE_MAP || !item.u.list) {
            continue;
        }

        MediaTrack track;
        const mpv_node_list* map = item.u.list;
        for (int j = 0; j < map->num; ++j) {
            const char* key = map->keys[j];
            const mpv_node& val = map->values[j];
            if (!key) {
                continue;
            }

            if (std::strcmp(key, "id") == 0) {
                if (val.format == MPV_FORMAT_INT64) {
                    track.id = static_cast<int>(val.u.int64);
                }
            } else if (std::strcmp(key, "type") == 0) {
                if (val.format == MPV_FORMAT_STRING && val.u.string) {
                    track.type = QString::fromUtf8(val.u.string);
                }
            } else if (std::strcmp(key, "title") == 0) {
                if (val.format == MPV_FORMAT_STRING && val.u.string) {
                    track.title = QString::fromUtf8(val.u.string);
                }
            } else if (std::strcmp(key, "lang") == 0) {
                if (val.format == MPV_FORMAT_STRING && val.u.string) {
                    track.language = QString::fromUtf8(val.u.string);
                }
            } else if (std::strcmp(key, "codec") == 0) {
                if (val.format == MPV_FORMAT_STRING && val.u.string) {
                    track.codec = QString::fromUtf8(val.u.string);
                }
            } else if (std::strcmp(key, "selected") == 0) {
                if (val.format == MPV_FORMAT_FLAG) {
                    track.selected = (val.u.flag != 0);
                } else if (val.format == MPV_FORMAT_INT64) {
                    track.selected = (val.u.int64 != 0);
                }
            } else if (std::strcmp(key, "default") == 0) {
                if (val.format == MPV_FORMAT_FLAG) {
                    track.isDefault = (val.u.flag != 0);
                } else if (val.format == MPV_FORMAT_INT64) {
                    track.isDefault = (val.u.int64 != 0);
                }
            }
        }
        result.append(std::move(track));
    }
    return result;
}

QList<ChapterInfo> parseChapterList(const mpv_node* node)
{
    QList<ChapterInfo> result;
    if (!node || node->format != MPV_FORMAT_NODE_ARRAY || !node->u.list) {
        return result;
    }

    const mpv_node_list* list = node->u.list;
    result.reserve(list->num);

    for (int i = 0; i < list->num; ++i) {
        const mpv_node& item = list->values[i];
        if (item.format != MPV_FORMAT_NODE_MAP || !item.u.list) {
            continue;
        }

        ChapterInfo chapter;
        chapter.index = i;
        const mpv_node_list* map = item.u.list;
        for (int j = 0; j < map->num; ++j) {
            const char* key = map->keys[j];
            const mpv_node& val = map->values[j];
            if (!key) {
                continue;
            }

            if (std::strcmp(key, "title") == 0) {
                if (val.format == MPV_FORMAT_STRING && val.u.string) {
                    chapter.title = QString::fromUtf8(val.u.string);
                }
            } else if (std::strcmp(key, "time") == 0) {
                if (val.format == MPV_FORMAT_DOUBLE) {
                    chapter.time = val.u.double_;
                } else if (val.format == MPV_FORMAT_INT64) {
                    chapter.time = static_cast<double>(val.u.int64);
                }
            }
        }
        result.append(std::move(chapter));
    }
    return result;
}

double parseRatio(const QString& str)
{
    if (str.isEmpty() || str == QStringLiteral("-1") || str == QStringLiteral("auto") || str == QStringLiteral("no")) {
        return -1.0;
    }
    if (str.contains(u':')) {
        const auto parts = str.split(u':');
        if (parts.size() == 2) {
            bool okW = false;
            bool okH = false;
            double w = parts[0].trimmed().toDouble(&okW);
            double h = parts[1].trimmed().toDouble(&okH);
            if (okW && okH && h > 0.0) {
                return w / h;
            }
        }
    } else if (str.contains(u'/')) {
        const auto parts = str.split(u'/');
        if (parts.size() == 2) {
            bool okW = false;
            bool okH = false;
            double w = parts[0].trimmed().toDouble(&okW);
            double h = parts[1].trimmed().toDouble(&okH);
            if (okW && okH && h > 0.0) {
                return w / h;
            }
        }
    }
    bool ok = false;
    double val = str.toDouble(&ok);
    return ok ? val : -1.0;
}

} // namespace

MpvEngine::MpvEngine(QObject* parent)
    : IPlaybackEngine(parent)
{
}

MpvEngine::~MpvEngine()
{
    if (m_mpv) {
        mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
        mpv_handle* handle = m_mpv;
        m_mpv = nullptr;
        mpv_terminate_destroy(handle);
    }
}

bool MpvEngine::initialize()
{
    if (m_mpv) {
        return true;
    }

    // libmpv strictly requires LC_NUMERIC to be set to "C"
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        return false;
    }

    mpv_set_option_string(m_mpv, "terminal", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=v");
    mpv_set_option_string(m_mpv, "vo", "libmpv,null");

    int status = mpv_initialize(m_mpv);
    if (status < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return false;
    }

    // Observe core playback properties
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "video-aspect-override", MPV_FORMAT_STRING);

    // Setup event wakeup notification for Qt event loop
    mpv_set_wakeup_callback(m_mpv, &MpvEngine::onMpvWakeup, this);

    return true;
}

void MpvEngine::onMpvWakeup(void* ctx)
{
    auto* self = static_cast<MpvEngine*>(ctx);
    if (!self) {
        return;
    }
    QMetaObject::invokeMethod(self, &MpvEngine::handleMpvEvents, Qt::QueuedConnection);
}

void MpvEngine::handleMpvEvents()
{
    if (!m_mpv) {
        return;
    }

    while (m_mpv) {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }
        processEvent(event);
    }
}

void MpvEngine::processEvent(mpv_event* event)
{
    if (!event) {
        return;
    }

    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        auto* prop = static_cast<mpv_event_property*>(event->data);
        if (!prop || !prop->name) {
            break;
        }
        if (std::strcmp(prop->name, "time-pos") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
                m_position = *static_cast<double*>(prop->data);
                emit positionChanged(m_position);
            } else if (!prop->data) {
                m_position = 0.0;
            }
        } else if (std::strcmp(prop->name, "duration") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
                m_duration = *static_cast<double*>(prop->data);
                emit durationChanged(m_duration);
            } else if (!prop->data) {
                m_duration = 0.0;
            }
        } else if (std::strcmp(prop->name, "pause") == 0) {
            if (prop->format == MPV_FORMAT_FLAG && prop->data) {
                bool paused = (*static_cast<int*>(prop->data) != 0);
                if (m_paused != paused) {
                    m_paused = paused;
                    emit stateChanged(m_paused);
                }
            }
        } else if (std::strcmp(prop->name, "volume") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
                double vol = *static_cast<double*>(prop->data);
                if (!qFuzzyCompare(m_volume, vol)) {
                    m_volume = vol;
                    emit volumeChanged(m_volume);
                }
            }
        } else if (std::strcmp(prop->name, "mute") == 0) {
            if (prop->format == MPV_FORMAT_FLAG && prop->data) {
                bool muted = (*static_cast<int*>(prop->data) != 0);
                if (m_muted != muted) {
                    m_muted = muted;
                    emit muteChanged(m_muted);
                }
            }
        } else if (std::strcmp(prop->name, "speed") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
                double spd = *static_cast<double*>(prop->data);
                if (!qFuzzyCompare(m_speed, spd)) {
                    m_speed = spd;
                    emit speedChanged(m_speed);
                }
            }
        } else if (std::strcmp(prop->name, "video-aspect-override") == 0) {
            if (prop->format == MPV_FORMAT_STRING && prop->data) {
                const char* val = *static_cast<char**>(prop->data);
                if (val) {
                    bool ok = false;
                    double mpvRatio = QString::fromUtf8(val).toDouble(&ok);
                    double curRatio = parseRatio(m_aspectRatio);
                    if (ok && (curRatio <= 0.0 || std::abs(mpvRatio - curRatio) > 0.01)) {
                        if (mpvRatio <= 0.0) {
                            m_aspectRatio.clear();
                        } else if (std::abs(mpvRatio - 16.0 / 9.0) < 0.01) {
                            m_aspectRatio = QStringLiteral("16:9");
                        } else if (std::abs(mpvRatio - 4.0 / 3.0) < 0.01) {
                            m_aspectRatio = QStringLiteral("4:3");
                        } else if (std::abs(mpvRatio - 2.35) < 0.02) {
                            m_aspectRatio = QStringLiteral("2.35:1");
                        } else {
                            m_aspectRatio = QString::fromUtf8(val);
                        }
                    }
                }
            }
        } else if (std::strcmp(prop->name, "track-list") == 0) {
            if (prop->format == MPV_FORMAT_NODE && prop->data) {
                auto* node = static_cast<mpv_node*>(prop->data);
                m_tracks = parseTrackList(node);
                emit tracksChanged();
            }
        } else if (std::strcmp(prop->name, "chapter-list") == 0) {
            if (prop->format == MPV_FORMAT_NODE && prop->data) {
                auto* node = static_cast<mpv_node*>(prop->data);
                m_chapters = parseChapterList(node);
                emit chaptersChanged();
            }
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED:
        emit fileLoaded();
        break;
    case MPV_EVENT_END_FILE:
        emit playbackFinished();
        break;
    default:
        break;
    }
}

bool MpvEngine::loadFile(const QString& url)
{
    if (!m_mpv) {
        return false;
    }

    QString targetUrl = url;
    if (targetUrl.startsWith(QStringLiteral("lavfi://"))) {
        targetUrl = QStringLiteral("av://lavfi:") + targetUrl.mid(8);
    }

    const QByteArray utf8 = targetUrl.toUtf8();
    const char* cmd[] = {"loadfile", utf8.constData(), nullptr};
    int err = mpv_command(m_mpv, cmd);
    return err >= 0;
}

void MpvEngine::play()
{
    if (!m_mpv) {
        return;
    }
    int flag = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &flag);
    if (m_paused) {
        m_paused = false;
        emit stateChanged(false);
    }
}

void MpvEngine::pause()
{
    if (!m_mpv) {
        return;
    }
    int flag = 1;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &flag);
    if (!m_paused) {
        m_paused = true;
        emit stateChanged(true);
    }
}

void MpvEngine::togglePause()
{
    if (!m_mpv) {
        return;
    }
    if (isPaused()) {
        play();
    } else {
        pause();
    }
}

void MpvEngine::stop()
{
    if (!m_mpv) {
        return;
    }
    const char* cmd[] = {"stop", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
    m_position = 0.0;
    m_duration = 0.0;
    m_tracks.clear();
    m_chapters.clear();
}

void MpvEngine::seek(double seconds, bool exact)
{
    if (!m_mpv) {
        return;
    }
    QByteArray posStr = QByteArray::number(seconds, 'f', 6);
    const char* mode = exact ? "absolute+exact" : "absolute";
    const char* cmd[] = {"seek", posStr.constData(), mode, nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

double MpvEngine::position() const
{
    return m_position;
}

double MpvEngine::duration() const
{
    return m_duration;
}

bool MpvEngine::isPaused() const
{
    return m_paused;
}

QList<MediaTrack> MpvEngine::tracks(const QString& type) const
{
    if (type.isEmpty()) {
        return m_tracks;
    }

    QList<MediaTrack> filtered;
    for (const auto& track : m_tracks) {
        if (track.type.compare(type, Qt::CaseInsensitive) == 0) {
            filtered.append(track);
        }
    }
    return filtered;
}

bool MpvEngine::setTrack(const QString& type, int trackId)
{
    if (!m_mpv) {
        return false;
    }

    const char* propName = nullptr;
    if (type.compare(QLatin1String("video"), Qt::CaseInsensitive) == 0) {
        propName = "vid";
    } else if (type.compare(QLatin1String("audio"), Qt::CaseInsensitive) == 0) {
        propName = "aid";
    } else if (type.compare(QLatin1String("sub"), Qt::CaseInsensitive) == 0 ||
               type.compare(QLatin1String("subtitle"), Qt::CaseInsensitive) == 0) {
        propName = "sid";
    } else {
        return false;
    }

    QByteArray val = (trackId <= 0) ? QByteArray("no") : QByteArray::number(trackId);
    return mpv_set_property_string(m_mpv, propName, val.constData()) >= 0;
}

double MpvEngine::volume() const
{
    return m_volume;
}

void MpvEngine::setVolume(double vol)
{
    double clamped = std::max(0.0, vol);
    m_volume = clamped;
    if (m_mpv) {
        mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &clamped);
    }
    emit volumeChanged(m_volume);
}

bool MpvEngine::isMuted() const
{
    return m_muted;
}

void MpvEngine::setMuted(bool mute)
{
    m_muted = mute;
    if (m_mpv) {
        int flag = mute ? 1 : 0;
        mpv_set_property_async(m_mpv, 0, "mute", MPV_FORMAT_FLAG, &flag);
    }
    emit muteChanged(m_muted);
}

double MpvEngine::speed() const
{
    return m_speed;
}

void MpvEngine::setSpeed(double spd)
{
    if (spd <= 0.0) {
        return;
    }
    m_speed = spd;
    if (m_mpv) {
        mpv_set_property_async(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE, &spd);
    }
    emit speedChanged(m_speed);
}

QString MpvEngine::aspectRatio() const
{
    return m_aspectRatio;
}

void MpvEngine::setAspectRatio(const QString& ar)
{
    m_aspectRatio = ar;
    if (m_mpv) {
        QByteArray val = (ar.isEmpty() || ar == QStringLiteral("-1")) ? QByteArray("-1") : ar.toUtf8();
        mpv_set_property_string(m_mpv, "video-aspect-override", val.constData());
    }
}

QList<ChapterInfo> MpvEngine::chapters() const
{
    return m_chapters;
}

bool MpvEngine::setChapter(int chapterIndex)
{
    if (!m_mpv || chapterIndex < 0) {
        return false;
    }
    if (!m_chapters.isEmpty() && chapterIndex >= m_chapters.size()) {
        return false;
    }
    int64_t idx = chapterIndex;
    return mpv_set_property_async(m_mpv, 0, "chapter", MPV_FORMAT_INT64, &idx) >= 0;
}

bool MpvEngine::setProperty(const QString& name, const QString& value)
{
    if (!m_mpv) {
        return false;
    }
    const QByteArray nameUtf8 = name.toUtf8();
    const QByteArray valUtf8 = value.toUtf8();
    return mpv_set_property_string(m_mpv, nameUtf8.constData(), valUtf8.constData()) >= 0;
}

QString MpvEngine::getPropertyString(const QString& name) const
{
    if (!m_mpv) {
        return {};
    }
    const QByteArray nameUtf8 = name.toUtf8();
    char* val = mpv_get_property_string(m_mpv, nameUtf8.constData());
    if (!val) {
        return {};
    }
    QString result = QString::fromUtf8(val);
    mpv_free(val);
    return result;
}

void MpvEngine::setVo(const QString& vo)
{
    setProperty(QStringLiteral("vo"), vo);
}

} // namespace outplayer::engine
