#include "core.h"

namespace sv {

	SV_DEFINE_HANDLE(EventListener);

    struct Event {
        EventListener* eventListener;
    };
    
    typedef std::function<void(const Event*)> EventFunction;

    class EventRegister {
    public:
        EventRegister() = default;
        ~EventRegister();

        EventRegister(const EventRegister& other) = delete;
        EventRegister(EventRegister&& other) noexcept;

        EventRegister& operator=(const EventRegister& other) = delete;
        EventRegister& operator=(EventRegister&& other) noexcept;

        void bindRaw(EventListener* eventListener, const EventFunction& fn);
        void unbind();

		template<typename T>
		inline void bind(EventListener* eventListener, const std::function<void(const T*)>& fn)
		{
			bindRaw(eventListener, reinterpret_cast<const EventFunction&>(fn));
		}

		EventListener* getEventListener() const noexcept;

    private:
		void* pInternal;

    };

	void event_listener_attach(EventListener* src, EventListener* dst, const std::function<bool(Event*)>& eventFn);
	void event_listener_detach(EventListener* src, EventListener* dst);

	EventListener*	event_listener_open();
	void			event_listener_close(EventListener* eventListener);

    void event_dispatch(EventListener* eventListener, Event* event);

}
