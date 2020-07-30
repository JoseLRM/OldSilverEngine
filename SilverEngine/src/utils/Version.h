#pragma once

#include "core.h"

namespace sv {

	struct Version {
		ui32 major = 0u;
		ui32 minor = 0u;
		ui32 revision = 0u;

		Version() = default;
		Version(ui32 major, ui32 minor, ui32 revision) 
			: major(major), minor(minor), revision(revision) {}

		std::string ToString() {
			std::stringstream ss;
			ss << major << '.' << minor << '.' << revision;
			return ss.str();
		}
		ui64 GetVersion()
		{
			return ui64(major) * 1000000L + ui64(minor) * 1000L + ui64(revision);
		}

		bool operator== (const Version& other) const noexcept
		{
			return major == other.major && minor == other.minor && revision == other.revision;
		}
		bool operator!= (const Version& other) const noexcept
		{
			return !this->operator==(other);
		}
		bool operator< (const Version& other) const noexcept
		{
			if (major != other.major) return major < other.major;
			else if (minor != other.minor) return minor < other.minor;
			else return revision < other.revision;
		}
		bool operator> (const Version& other) const noexcept
		{
			if (major != other.major) return major > other.major;
			else if (minor != other.minor) return minor > other.minor;
			else return revision > other.revision;
		}
		bool operator<= (const Version& other) const noexcept {
			return this->operator<(other) || this->operator==(other);
		}
		bool operator>= (const Version& other) const noexcept {
			return this->operator>(other) || this->operator==(other);
		}

	};

}