#pragma once

#include "logic/heard_line.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace HeardLines
{
	// Keeps scene lines and menu talk. Drops the player's own lines, Mantella's lines (Mantella.esp), and chatter:
	// greetings (also the one when the dialogue menu opens), idle remarks, combat shouts, grunts and breathing outside scenes
	[[nodiscard]] bool IsWorthKeeping(const LineFacts& a_line);

	// The game may report one line more than once (once per response). Lets a line through once while it is the
	// speaker's latest
	class RepeatGuard
	{
	public:
		[[nodiscard]] bool IsNew(std::uint32_t a_speaker, std::uint32_t a_info);
		void               Clear();

	private:
		std::mutex                                       _lock;
		std::unordered_map<std::uint32_t, std::uint32_t> _latest;  // Speaker -> their latest line
	};
}
