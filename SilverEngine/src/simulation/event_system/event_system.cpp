#include "core.h"

#include "event_system_internal.h"
#include "utils/allocator.h"

namespace sv {

#define PARSE_LISTENER() sv::EventListener_internal& listener = *reinterpret_cast<sv::EventListener_internal*>(listener_);

	static InstanceAllocator<EventListener_internal> g_Listeners;
	static InstanceAllocator<EventRegister_internal> g_Registers;

    // MAIN FUNCTIONS

    Result event_initialize()
    {
        return Result_Success;
    }

    Result event_close()
    {
		ui32 unfreed = g_Registers.unfreed_count();
		if (unfreed) SV_LOG_ERROR("There are %u unbound event registers", unfreed);

		unfreed = g_Listeners.unfreed_count();
		if (unfreed) SV_LOG_ERROR("There are %u unfreed event listeners", unfreed);

		g_Listeners.clear();
        return Result_Success;
    }

    // EVENT REGISTER

    EventRegister::~EventRegister()
    {
		if (!g_Registers.empty())
			unbind();
    }

    EventRegister::EventRegister(EventRegister&& other) noexcept
    {
		pInternal = other.pInternal;
		other.pInternal = nullptr;
    }

    EventRegister& EventRegister::operator=(EventRegister&& other) noexcept
    {
		unbind();
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
    }

    void EventRegister::bindRaw(EventListener* listener_, const EventFunction& fn)
    {
		unbind();

		if (listener_ != nullptr)  {
			PARSE_LISTENER();
			EventRegister_internal& reg = *g_Registers.create();
			reg.listener = &listener;
			std::scoped_lock<std::shared_mutex> lock(listener.mutex);
			listener.functions.push_back({ fn, &reg });
			pInternal = &reg;
		}
    }

    void EventRegister::unbind()
    {
		if (pInternal == nullptr) return;
        
		EventRegister_internal& reg = *reinterpret_cast<EventRegister_internal*>(pInternal);
		pInternal = nullptr;

		if (reg.listener) {
			auto& listener = *reg.listener;

			listener.mutex.lock_shared();

			for (auto it = listener.functions.begin(); it != listener.functions.end(); ++it) {
				if (it->pRegister == &reg) {
					listener.mutex.unlock_shared();
					{
						std::scoped_lock lock2(listener.mutex);
						listener.functions.erase(it);
						reg.listener = nullptr;
					}
					break;
				}
			}
			if (reg.listener) {
				SV_LOG_ERROR("Can't unbind register, not found in listener. That's really bad..");
				listener.mutex.unlock_shared();
			}
		}

		g_Registers.destroy(&reg);
    }

	EventListener* EventRegister::getEventListener() const noexcept
	{
		return nullptr;
	}

    // LISTENER

	void event_listener_attach(EventListener* src_, EventListener* dst_, const std::function<bool(Event*)>& eventFn)
	{
		if (src_ == nullptr || dst_ == nullptr) return;

		EventListener_internal& src = *reinterpret_cast<EventListener_internal*>(src_);
		EventListener_internal& dst = *reinterpret_cast<EventListener_internal*>(dst_);

		// Check if must attach

		if (src.attachments.size() + src.attached.size() <= dst.attachments.size() dst.attached.size()) {
			for (EventListenerAttachment& att : src.attachments) {
				if (att.attachment == &dst) {
					SV_LOG_ERROR("Can't attach because it is currently attached");
					return;
				}
			}
			for (EventListener_internal* att : src.attached) {
				if (att == &dst) {
					SV_LOG_ERROR("Can't attach because it is currently attached");
					return;
				}
			}
		}
		else {
			for (EventListenerAttachment& att : dst.attachments) {
				if (att.attachment == &src) {
					SV_LOG_ERROR("Can't attach because it is currently attached");
					return;
				}
			}
			for (EventListener_internal* att : dst.attached) {
				if (att == &src) {
					SV_LOG_ERROR("Can't attach because it is currently attached");
					return;
				}
			}
		}

		// Attach
		{
			std::scoped_lock lock(src.mutex);

			auto& att = src.attachments.emplace_back();
			att.attachment = &dst;
			att.fn = eventFn;
		}
		{
			std::scoped_lock lock(dst.mutex);

			auto& att = dst.attached.emplace_back();
			att = &src;
		}
	}

	void event_listener_detach(EventListener* src_, EventListener* dst_)
	{
		if (src_ == nullptr || dst_ == nullptr) return;

		EventListener_internal& src = *reinterpret_cast<EventListener_internal*>(src_);
		EventListener_internal& dst = *reinterpret_cast<EventListener_internal*>(dst_);

		{
			std::scoped_lock lock(src.mutex);

			for (auto it = src.attachments.begin(); it != src.attachments.end(); ++it) {
				if (it->attachment == &dst) {
					src.attachments.erase(it);
					break;
				}
			}
		}
		{
			std::scoped_lock lock(dst.mutex);

			for (auto it = dst.attached.begin(); it != dst.attached.end(); ++it) {
				if (*it == &src) {
					dst.attached.erase(it);
					break;
				}
			}
		}
	}

	EventListener* event_listener_open()
    {
        return (EventListener*)g_Listeners.create();
    }

	void event_listener_close(EventListener* listener_)
	{
		if (listener_ == nullptr) return;
		PARSE_LISTENER();

		std::scoped_lock lock(listener.mutex);

		for (auto& reg : listener.functions) {
			reg.pRegister->listener = nullptr;
		}

		for (auto& att : listener.attached) {
			std::scoped_lock lock2(att->mutex);
			
			for (auto it = att->attachments.begin(); it != att->attachments.end(); ++it) {
				if (it->attachment == &listener) {
					att->attachments.erase(it);
					break;
				}
			}
		}

		g_Listeners.destroy(&listener);
	}

    void event_dispatch(EventListener* listener_, Event* event)
    {
        if (listener_ == nullptr) return;
		event->eventListener = listener_;

		PARSE_LISTENER();

        std::shared_lock lock(listener.mutex);

        auto& fns = listener.functions;
        for (auto it = fns.begin(); it != fns.end(); ++it) {
            SV_ASSERT(it->fn != nullptr);
			it->fn(event);
        }
        
		for (auto& att : listener.attachments) {
			if (att.fn(event)) {
				event_dispatch((EventListener*)att.attachment, event);
			}
		}
    }

}