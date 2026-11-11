// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/MpvEngine.h"

#include <QMetaObject>

#include <clocale>
#include <cstring>

namespace outplayer::engine {

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
    QMetaObject::invokeMethod(self, "handleMpvEvents", Qt::QueuedConnection);
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

    const QByteArray utf8 = url.toUtf8();
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
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
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
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
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
    mpv_command(m_mpv, cmd);
    m_position = 0.0;
    m_duration = 0.0;
}

void MpvEngine::seek(double seconds, bool exact)
{
    if (!m_mpv) {
        return;
    }
    QByteArray posStr = QByteArray::number(seconds, 'f', 6);
    const char* mode = exact ? "absolute+exact" : "absolute";
    const char* cmd[] = {"seek", posStr.constData(), mode, nullptr};
    mpv_command(m_mpv, cmd);
}

double MpvEngine::position() const
{
    if (m_mpv) {
        double pos = 0.0;
        if (mpv_get_property(m_mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0) {
            return pos;
        }
    }
    return m_position;
}

double MpvEngine::duration() const
{
    if (m_mpv) {
        double dur = 0.0;
        if (mpv_get_property(m_mpv, "duration", MPV_FORMAT_DOUBLE, &dur) >= 0) {
            return dur;
        }
    }
    return m_duration;
}

bool MpvEngine::isPaused() const
{
    if (m_mpv) {
        int flag = 0;
        if (mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag) >= 0) {
            return flag != 0;
        }
    }
    return m_paused;
}

} // namespace outplayer::engine
