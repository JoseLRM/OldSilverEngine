#include "core.h"

namespace sv {
	
	void throw_assertion(const char* content, ui32 line, const char* file)
	{
#ifdef SV_ENABLE_LOGGING
		__internal__do_not_call_this_please_or_you_will_die__console_log(4u, "[ASSERTION] '%s', file: '%s', line: %u", content, file, line);
#endif
		std::wstringstream ss;
		ss << L"'";
		ss << parse_wstring(content);
		ss << L"'. File: '";
		ss << parse_wstring(file);
		ss << L"'. Line: " << line;
		ss << L". Do you want to continue?";

		if (sv::show_message(L"ASSERTION!!", ss.str().c_str(), sv::MessageStyle_IconError | sv::MessageStyle_YesNo) == 1)
		{
			exit(1u);
		}
	}
	
}
