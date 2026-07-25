#pragma once
#include "archpch.h"

namespace Engine {
	enum class EventType {
		None = 0,
		WindowClose, WindowResize,
		KeyPressed, KeyReleased,
		MouseMoved, MouseButtonPressed, MouseButtonReleased, MouseScrolled
	};

	class Event {
	public:
		virtual ~Event() = default;
		virtual EventType GetType() const = 0;
		virtual const char* GetName() const = 0;

		bool Handled = false;
	};

    #define EVENT_CLASS_TYPE(type) \
        static EventType GetStaticType() { return EventType::type; } \
        virtual EventType GetType() const override { return GetStaticType(); } \
        virtual const char* GetName() const override { return #type; }

    class EventDispatcher {
    public:
        EventDispatcher(Event& event) : _Event(event) {}

        // F should be a callable taking (T&) and returning bool (true = handled)
        template<typename T, typename F>
        bool Dispatch(const F& func) {
            if (_Event.GetType() == T::GetStaticType()) {
                _Event.Handled |= func(static_cast<T&>(_Event));
                return true;
            }
            return false;
        }

    private:
        Event& _Event;
    };
}
