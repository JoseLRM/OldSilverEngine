#pragma once

#include "core.h"

namespace sv {

	struct Version {
		u32 major = 0u;
		u32 minor = 0u;
		u32 revision = 0u;

		constexpr Version() = default;
		constexpr Version(u32 major, u32 minor, u32 revision) 
			: major(major), minor(minor), revision(revision) {}

		std::string toString() const noexcept {
			std::stringstream ss;
			ss << major << '.' << minor << '.' << revision;
			return ss.str();
		}
		u64 getVersion() const noexcept
		{
			return u64(major) * 1000000L + u64(minor) * 1000L + u64(revision);
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