// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/MpvEngine.h"
#include "core/SettingsManager.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QStringList>

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
    mpv_set_option_string(m_mpv, "sub-auto", "fuzzy");

    int status = mpv_initialize(m_mpv);
    if (status < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return false;
    }

    mpv_set_property_string(m_mpv, "sub-visibility", "yes");

    // Load and apply persisted preferences and subtitle appearance settings
    auto& settings = core::SettingsManager::instance();

    const QString hwdec = settings.hardwareDecoder();
    if (!hwdec.isEmpty()) {
        mpv_set_property_string(m_mpv, "hwdec", hwdec.toUtf8().constData());
    }

    setVolume(settings.defaultVolume());
    setSpeed(settings.get(QStringLiteral("Playback/DefaultSpeed"), 1.0).toDouble());

    const QString aspectRatioPref = settings.get(QStringLiteral("Video/AspectRatio"), QStringLiteral("auto")).toString();
    if (!aspectRatioPref.isEmpty() && aspectRatioPref.compare(QStringLiteral("auto"), Qt::CaseInsensitive) != 0) {
        setAspectRatio(aspectRatioPref);
    }

    const QVariant deintVar = settings.contains(QStringLiteral("Video/Deinterlacing"))
        ? settings.get(QStringLiteral("Video/Deinterlacing"))
        : settings.get(QStringLiteral("Video/Deinterlace"));
    const QString deintStr = deintVar.toString().trimmed().toLower();
    if (deintVar.toBool() || deintStr == QStringLiteral("yes") || deintStr == QStringLiteral("true")) {
        setDeinterlace(true);
    }

    if (settings.get(QStringLiteral("Audio/Normalizer"), false).toBool()) {
        setNormalizerEnabled(true);
    }

    const QString subLang = settings.get(QStringLiteral("Subtitles/DefaultLanguage"), QString()).toString().trimmed();
    if (!subLang.isEmpty()) {
        mpv_set_property_string(m_mpv, "slang", subLang.toUtf8().constData());
    }

    m_subtitleFont = settings.get(QStringLiteral("Subtitles/Font"), QStringLiteral("sans-serif")).toString();
    m_subtitleFontSize = settings.get(QStringLiteral("Subtitles/FontSize"), 24).toInt();

    const QString colorStr = settings.get(QStringLiteral("Subtitles/Color"), QStringLiteral("#ffffffff")).toString();
    if (QColor::isValidColorName(colorStr)) {
        m_subtitleColor = QColor(colorStr);
    } else {
        m_subtitleColor = QColor(255, 255, 255, 255);
    }

    m_subtitleBold = settings.get(QStringLiteral("Subtitles/Bold"), false).toBool();
    m_subtitleItalic = settings.get(QStringLiteral("Subtitles/Italic"), false).toBool();

    const QString borderColStr = settings.get(QStringLiteral("Subtitles/BorderColor"), QStringLiteral("#ff000000")).toString();
    if (QColor::isValidColorName(borderColStr)) {
        m_subtitleBorderColor = QColor(borderColStr);
    } else {
        m_subtitleBorderColor = QColor(0, 0, 0, 255);
    }

    m_subtitleBorderSize = settings.get(QStringLiteral("Subtitles/BorderSize"), 2.0).toDouble();

    const QString shadowColStr = settings.get(QStringLiteral("Subtitles/ShadowColor"), QStringLiteral("#80000000")).toString();
    if (QColor::isValidColorName(shadowColStr)) {
        m_subtitleShadowColor = QColor(shadowColStr);
    } else {
        m_subtitleShadowColor = QColor(0, 0, 0, 128);
    }

    m_subtitleShadowOffset = settings.get(QStringLiteral("Subtitles/ShadowOffset"), 0.0).toDouble();

    setSubtitleFont(m_subtitleFont);
    setSubtitleFontSize(m_subtitleFontSize);
    setSubtitleColor(m_subtitleColor);
    setSubtitleBold(m_subtitleBold);
    setSubtitleItalic(m_subtitleItalic);
    setSubtitleBorderColor(m_subtitleBorderColor);
    setSubtitleBorderSize(m_subtitleBorderSize);
    setSubtitleShadowColor(m_subtitleShadowColor);
    setSubtitleShadowOffset(m_subtitleShadowOffset);

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
    mpv_observe_property(m_mpv, 0, "sub-visibility", MPV_FORMAT_FLAG);

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
        } else if (std::strcmp(prop->name, "sub-visibility") == 0) {
            if (prop->format == MPV_FORMAT_FLAG && prop->data) {
                bool visible = (*static_cast<int*>(prop->data) != 0);
                if (m_subtitleVisible != visible) {
                    m_subtitleVisible = visible;
                    emit subtitleSettingsChanged();
                }
            }
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED:
        emit fileLoaded();
        break;
    case MPV_EVENT_END_FILE: {
        const auto* endFile = static_cast<const mpv_event_end_file*>(event->data);
        if (!endFile || endFile->reason == MPV_END_FILE_REASON_EOF) {
            emit playbackFinished();
        } else if (endFile->reason == MPV_END_FILE_REASON_ERROR) {
            emit playbackStopped();
            emit errorOccurred(QString::fromUtf8(mpv_error_string(endFile->error)));
        } else if (endFile->reason == MPV_END_FILE_REASON_STOP ||
                   endFile->reason == MPV_END_FILE_REASON_QUIT) {
            emit playbackStopped();
        }
        break;
    }
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
        QString graph = targetUrl.mid(8);
        graph.replace(QStringLiteral("[v]"), QStringLiteral("[out0]"));
        graph.replace(QStringLiteral("[a]"), QStringLiteral("[out1]"));
        targetUrl = QStringLiteral("av://lavfi:") + graph;
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
    emit positionChanged(0.0);
    emit durationChanged(0.0);
    emit tracksChanged();
    emit chaptersChanged();
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

void MpvEngine::frameStep()
{
    if (!m_mpv) {
        return;
    }
    const char* cmd[] = {"frame-step", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvEngine::frameBackStep()
{
    if (!m_mpv) {
        return;
    }
    const char* cmd[] = {"frame-back-step", nullptr};
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

    QString normalizedType = type;
    if (normalizedType.compare(QLatin1String("subtitle"), Qt::CaseInsensitive) == 0) {
        normalizedType = QStringLiteral("sub");
    }

    QList<MediaTrack> filtered;
    for (const auto& track : m_tracks) {
        if (track.type.compare(normalizedType, Qt::CaseInsensitive) == 0) {
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

void MpvEngine::setHardwareDecoder(const QString& hwdec)
{
    if (!hwdec.isEmpty()) {
        setProperty(QStringLiteral("hwdec"), hwdec);
    }
}

void MpvEngine::setSubtitleLanguage(const QString& lang)
{
    const QString trimmed = lang.trimmed();
    if (!trimmed.isEmpty()) {
        setProperty(QStringLiteral("slang"), trimmed);
    }
}

void MpvEngine::setAspectRatio(const QString& ar)
{
    m_aspectRatio = ar;
    if (m_mpv) {
        const bool isAuto = ar.isEmpty()
            || ar == QStringLiteral("-1")
            || ar.compare(QStringLiteral("auto"), Qt::CaseInsensitive) == 0
            || ar.compare(QStringLiteral("no"), Qt::CaseInsensitive) == 0;
        const QByteArray val = isAuto ? QByteArrayLiteral("no") : ar.toUtf8();
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

int MpvEngine::brightness() const
{
    return m_brightness;
}

void MpvEngine::setBrightness(int val)
{
    m_brightness = std::clamp(val, -100, 100);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "brightness", QByteArray::number(m_brightness).constData());
    }
    emit videoEqualizerChanged();
}

int MpvEngine::contrast() const
{
    return m_contrast;
}

void MpvEngine::setContrast(int val)
{
    m_contrast = std::clamp(val, -100, 100);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "contrast", QByteArray::number(m_contrast).constData());
    }
    emit videoEqualizerChanged();
}

int MpvEngine::saturation() const
{
    return m_saturation;
}

void MpvEngine::setSaturation(int val)
{
    m_saturation = std::clamp(val, -100, 100);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "saturation", QByteArray::number(m_saturation).constData());
    }
    emit videoEqualizerChanged();
}

int MpvEngine::hue() const
{
    return m_hue;
}

void MpvEngine::setHue(int val)
{
    m_hue = std::clamp(val, -100, 100);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "hue", QByteArray::number(m_hue).constData());
    }
    emit videoEqualizerChanged();
}

int MpvEngine::rotation() const
{
    return m_rotation;
}

void MpvEngine::setRotation(int degrees)
{
    int norm = ((degrees % 360) + 360) % 360;
    norm = ((norm + 45) / 90 * 90) % 360;
    m_rotation = norm;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "video-rotate", QByteArray::number(m_rotation).constData());
    }
    emit videoEqualizerChanged();
}

bool MpvEngine::deinterlace() const
{
    return m_deinterlace;
}

void MpvEngine::setDeinterlace(bool enable)
{
    m_deinterlace = enable;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "deinterlace", enable ? "yes" : "no");
    }
    emit videoEqualizerChanged();
}

void MpvEngine::resetVideoEqualizer()
{
    setBrightness(0);
    setContrast(0);
    setSaturation(0);
    setHue(0);
}

double MpvEngine::audioDelay() const
{
    return m_audioDelay;
}

void MpvEngine::setAudioDelay(double seconds)
{
    m_audioDelay = seconds;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "audio-delay", QByteArray::number(m_audioDelay, 'f', 3).constData());
    }
    emit audioEqualizerChanged();
}

double MpvEngine::equalizerBand(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= static_cast<int>(m_eqBands.size())) {
        return 0.0;
    }
    return m_eqBands[bandIndex];
}

void MpvEngine::setEqualizerBand(int bandIndex, double gainDb)
{
    if (bandIndex < 0 || bandIndex >= static_cast<int>(m_eqBands.size())) {
        return;
    }
    m_eqBands[bandIndex] = std::clamp(gainDb, -12.0, 12.0);
    updateAudioFilters();
    emit audioEqualizerChanged();
}

void MpvEngine::resetAudioEqualizer()
{
    m_eqBands.fill(0.0);
    updateAudioFilters();
    emit audioEqualizerChanged();
}

bool MpvEngine::isNormalizerEnabled() const
{
    return m_normalizerEnabled;
}

void MpvEngine::setNormalizerEnabled(bool enable)
{
    m_normalizerEnabled = enable;
    updateAudioFilters();
    emit audioEqualizerChanged();
}

void MpvEngine::updateAudioFilters()
{
    if (!m_mpv) {
        return;
    }
    QStringList filters;

    bool hasEq = false;
    for (double gain : m_eqBands) {
        if (std::abs(gain) > 1e-3) {
            hasEq = true;
            break;
        }
    }

    if (hasEq) {
        static constexpr int kBandFrequencies[10] = {
            31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000
        };
        QStringList entries;
        entries.reserve(10);
        for (int i = 0; i < 10; ++i) {
            entries.append(QStringLiteral("entry(%1,%2)")
                               .arg(kBandFrequencies[i])
                               .arg(m_eqBands[i], 0, 'f', 1));
        }
        filters.append(QStringLiteral("lavfi=[firequalizer=gain_entry='%1']").arg(entries.join(u';')));
    }

    if (m_normalizerEnabled) {
        filters.append(QStringLiteral("lavfi=[dynaudnorm]"));
    }

    QString afStr = filters.join(u',');
    QByteArray afBytes = afStr.toUtf8();
    mpv_set_property_string(m_mpv, "af", afBytes.constData());
}

double MpvEngine::subtitleDelay() const
{
    return m_subtitleDelay;
}

void MpvEngine::setSubtitleDelay(double seconds)
{
    m_subtitleDelay = seconds;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-delay", QByteArray::number(m_subtitleDelay, 'f', 3).constData());
    }
    emit subtitleSettingsChanged();
}

double MpvEngine::subtitleScale() const
{
    return m_subtitleScale;
}

void MpvEngine::setSubtitleScale(double scale)
{
    m_subtitleScale = std::clamp(scale, 0.5, 3.0);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-scale", QByteArray::number(m_subtitleScale, 'f', 2).constData());
    }
    emit subtitleSettingsChanged();
}

bool MpvEngine::isSubtitleVisible() const
{
    if (m_mpv) {
        int flag = 1;
        if (mpv_get_property(m_mpv, "sub-visibility", MPV_FORMAT_FLAG, &flag) >= 0) {
            return flag != 0;
        }
    }
    return m_subtitleVisible;
}

void MpvEngine::setSubtitleVisible(bool visible)
{
    m_subtitleVisible = visible;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-visibility", visible ? "yes" : "no");
    }
    emit subtitleSettingsChanged();
}

int MpvEngine::currentSubtitleTrack() const
{
    if (m_mpv) {
        char* val = mpv_get_property_string(m_mpv, "sid");
        if (val) {
            QString s = QString::fromUtf8(val);
            mpv_free(val);
            bool ok = false;
            int id = s.toInt(&ok);
            if (ok && id > 0) {
                return id;
            }
            if (s == QStringLiteral("no")) {
                return 0;
            }
        }
    }
    for (const auto& track : m_tracks) {
        if (track.type == QLatin1String("sub") && track.selected) {
            return track.id;
        }
    }
    return 0;
}

bool MpvEngine::setSubtitleTrack(int trackId)
{
    return setTrack(QStringLiteral("sub"), trackId);
}

bool MpvEngine::loadSubtitle(const QString& filePath)
{
    if (!m_mpv || filePath.isEmpty()) {
        return false;
    }
    const QByteArray pathUtf8 = filePath.toUtf8();
    const char* cmd[] = {"sub-add", pathUtf8.constData(), "select", nullptr};
    return mpv_command(m_mpv, cmd) >= 0;
}

bool MpvEngine::captureFrame(const QString& destinationPath)
{
    if (!m_mpv || destinationPath.isEmpty()) {
        return false;
    }
    QFileInfo fi(destinationPath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    const QByteArray pathUtf8 = destinationPath.toUtf8();
    const char* cmd[] = {"screenshot-to-file", pathUtf8.constData(), "video", nullptr};
    return mpv_command(m_mpv, cmd) >= 0;
}

QString MpvEngine::subtitleFont() const
{
    return m_subtitleFont;
}

void MpvEngine::setSubtitleFont(const QString& fontName)
{
    m_subtitleFont = fontName.trimmed().isEmpty() ? QStringLiteral("sans-serif") : fontName.trimmed();
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-font", m_subtitleFont.toUtf8().constData());
    }
    emit subtitleSettingsChanged();
}

int MpvEngine::subtitleFontSize() const
{
    return m_subtitleFontSize;
}

void MpvEngine::setSubtitleFontSize(int pt)
{
    m_subtitleFontSize = std::clamp(pt, 10, 96);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-font-size", QByteArray::number(m_subtitleFontSize).constData());
    }
    emit subtitleSettingsChanged();
}

QColor MpvEngine::subtitleColor() const
{
    return m_subtitleColor;
}

void MpvEngine::setSubtitleColor(const QColor& color)
{
    if (!color.isValid()) {
        return;
    }
    m_subtitleColor = color;
    if (m_mpv) {
        const QString hex = color.name(QColor::HexArgb);
        mpv_set_property_string(m_mpv, "sub-color", hex.toUtf8().constData());
    }
    emit subtitleSettingsChanged();
}

bool MpvEngine::isSubtitleBold() const
{
    return m_subtitleBold;
}

void MpvEngine::setSubtitleBold(bool bold)
{
    m_subtitleBold = bold;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-bold", bold ? "yes" : "no");
    }
    emit subtitleSettingsChanged();
}

bool MpvEngine::isSubtitleItalic() const
{
    return m_subtitleItalic;
}

void MpvEngine::setSubtitleItalic(bool italic)
{
    m_subtitleItalic = italic;
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-italic", italic ? "yes" : "no");
    }
    emit subtitleSettingsChanged();
}

QColor MpvEngine::subtitleBorderColor() const
{
    return m_subtitleBorderColor;
}

void MpvEngine::setSubtitleBorderColor(const QColor& color)
{
    if (!color.isValid()) {
        return;
    }
    m_subtitleBorderColor = color;
    if (m_mpv) {
        const QString hex = color.name(QColor::HexArgb);
        mpv_set_property_string(m_mpv, "sub-border-color", hex.toUtf8().constData());
    }
    emit subtitleSettingsChanged();
}

double MpvEngine::subtitleBorderSize() const
{
    return m_subtitleBorderSize;
}

void MpvEngine::setSubtitleBorderSize(double size)
{
    m_subtitleBorderSize = std::clamp(size, 0.0, 10.0);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-border-size", QByteArray::number(m_subtitleBorderSize, 'f', 1).constData());
    }
    emit subtitleSettingsChanged();
}

QColor MpvEngine::subtitleShadowColor() const
{
    return m_subtitleShadowColor;
}

void MpvEngine::setSubtitleShadowColor(const QColor& color)
{
    if (!color.isValid()) {
        return;
    }
    m_subtitleShadowColor = color;
    if (m_mpv) {
        const QString hex = color.name(QColor::HexArgb);
        mpv_set_property_string(m_mpv, "sub-shadow-color", hex.toUtf8().constData());
    }
    emit subtitleSettingsChanged();
}

double MpvEngine::subtitleShadowOffset() const
{
    return m_subtitleShadowOffset;
}

void MpvEngine::setSubtitleShadowOffset(double offset)
{
    m_subtitleShadowOffset = std::clamp(offset, 0.0, 10.0);
    if (m_mpv) {
        mpv_set_property_string(m_mpv, "sub-shadow-offset", QByteArray::number(m_subtitleShadowOffset, 'f', 1).constData());
    }
    emit subtitleSettingsChanged();
}

} // namespace outplayer::engine
