#pragma once
#include "Event.h"

namespace Engine {

    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(float x, float y) : _X(x), _Y(y) {}
        float GetX() const { return _X; }
        float GetY() const { return _Y; }
        EVENT_CLASS_TYPE(MouseMoved)
    private:
        float _X, _Y;
    };

    class MouseButtonPressedEvent : public Event {
    public:
        MouseButtonPressedEvent(int button) : _Button(button) {}
        int GetButton() const { return _Button; }
        EVENT_CLASS_TYPE(MouseButtonPressed)
    private:
        int _Button;
    };

    class MouseButtonReleasedEvent : public Event {
    public:
        MouseButtonReleasedEvent(int button) : _Button(button) {}
        int GetButton() const { return _Button; }
        EVENT_CLASS_TYPE(MouseButtonReleased)
    private:
        int _Button;
    };

    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(float delta) : _Delta(delta) {}
        float GetDelta() const { return _Delta; }
        EVENT_CLASS_TYPE(MouseScrolled)
    private:
        float _Delta;
    };
}