#pragma once
#include "Event.h"

namespace Engine {

    class KeyEvent : public Event {
    public:
        int GetKeyCode() const { return _KeyCode; }
    protected:
        KeyEvent(int keyCode) : _KeyCode(keyCode) {}
        int _KeyCode;
    };

    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(int keyCode, bool isRepeat) : KeyEvent(keyCode), _IsRepeat(isRepeat) {}
        bool IsRepeat() const { return _IsRepeat; }
        EVENT_CLASS_TYPE(KeyPressed)
    private:
        bool _IsRepeat;
    };

    class KeyReleasedEvent : public KeyEvent {
    public:
        KeyReleasedEvent(int keyCode) : KeyEvent(keyCode) {}
        EVENT_CLASS_TYPE(KeyReleased)
    };
}