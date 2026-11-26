#pragma once

#include "logic/heard_line.h"

#include <optional>

namespace HeardLines
{
	// Turns what the game reports about a line into plain data
	class GameLineReader
	{
	public:
		// What the filter needs. Cheap: called for every line any NPC starts, grunts included
		[[nodiscard]] static LineFacts ReadFacts(RE::TESObjectREFR* a_speaker, RE::TESTopicInfo* a_info);

		// The whole line, once it passed the filter. Nobody has heard it yet. Empty when it has no words
		[[nodiscard]] static std::optional<HeardLine> ReadLine(RE::TESObjectREFR* a_speaker, RE::TESTopicInfo* a_info, const LineFacts& a_facts);

		[[nodiscard]] static Person   PersonOf(RE::TESObjectREFR* a_ref);
		[[nodiscard]] static Point    PointOf(const RE::NiPoint3& a_point);
		[[nodiscard]] static GameTime Now();
		// False while the game can't tell the date, such as during loading (Calendar::GetDaysPassed then returns 1.0)
		[[nodiscard]] static bool IsTimeKnown();
	};
}
