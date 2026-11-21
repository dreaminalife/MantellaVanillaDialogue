#include "logic/line_filter.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace HeardLines
{
	namespace
	{
		bool EqualsIgnoringCase(std::string_view a_left, std::string_view a_right)
		{
			return a_left.size() == a_right.size() && std::equal(a_left.begin(), a_left.end(), a_right.begin(), [](char a, char b) {
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
			});
		}
	}

	bool IsWorthKeeping(const LineFacts& a_line)
	{
		if (a_line.speakerIsPlayer || EqualsIgnoringCase(a_line.fileName, "Mantella.esp")) {
			return false;
		}
		// The greeting when the dialogue menu opens isn't an answer to anything the player picked, also when the speaker is in a scene
		if (a_line.menuTalk) {
			return a_line.subtype != kHelloSubtype;
		}
		return a_line.subtype == kSceneSubtype || a_line.sceneId != 0;
	}

	bool RepeatGuard::IsNew(std::uint32_t a_speaker, std::uint32_t a_info)
	{
		std::scoped_lock lock(_lock);
		auto [it, added] = _latest.try_emplace(a_speaker, a_info);
		if (added) {
			return true;
		}
		if (it->second == a_info) {
			return false;
		}
		it->second = a_info;
		return true;
	}

	void RepeatGuard::Clear()
	{
		std::scoped_lock lock(_lock);
		_latest.clear();
	}
}
