#pragma once
#include "Event.h"

namespace Engine {

    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height) : _Width(width), _Height(height) {}

        uint32_t GetWidth() const { return _Width; }
        uint32_t GetHeight() const { return _Height; }

        EVENT_CLASS_TYPE(WindowResize)

    private:
        uint32_t _Width, _Height;
    };

    class WindowCloseEvent : public Event {
    public:
        WindowCloseEvent() = default;
        EVENT_CLASS_TYPE(WindowClose)
    };
}