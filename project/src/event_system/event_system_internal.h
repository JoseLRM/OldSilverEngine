#include "SilverEngine/event_system.h"

namespace sv {

	struct EventListener_internal;

	struct EventRegister_internal {
		EventListener_internal* listener = nullptr;
	};

	struct EventFunction_internal {
		EventFunction fn;
		EventRegister_internal* pRegister;
	};

	struct EventListenerAttachment {
		EventListener_internal* attachment;
		std::function<bool(Event*)> fn;
	};

    struct EventListener_internal {
        std::vector<EventFunction_internal> functions;
		std::vector<EventListenerAttachment> attachments;
		std::vector<EventListener_internal*> attached;
        std::shared_mutex mutex;
    };

    Result event_initialize();
    Result event_close();

}