// Which lines are kept: scene lines and menu talk, never the player's, Mantella's or chatter
#include "logic/line_filter.h"

#include <doctest/doctest.h>

using namespace HeardLines;

namespace
{
	LineFacts Line(int a_subtype, std::uint32_t a_scene = 0, bool a_menu = false, std::string a_file = "Skyrim.esm", bool a_player = false)
	{
		return { a_player, std::move(a_file), a_subtype, a_scene, a_menu };
	}
}

TEST_CASE("scene lines are kept")
{
	CHECK(IsWorthKeeping(Line(kSceneSubtype)));
}

TEST_CASE("a line from someone in a scene is kept, whatever Skyrim files it under")
{
	CHECK(IsWorthKeeping(Line(94, 0x000ABCDE)));
}

TEST_CASE("chatter outside scenes is dropped")
{
	for (const int subtype : { 79 /* greeting */, 94 /* idle */, 26, 40, 64 /* combat */, 101 /* grunt */, 95, 96, 97, 100, 102 /* breathing */ }) {
		CAPTURE(subtype);
		CHECK_FALSE(IsWorthKeeping(Line(subtype)));
	}
}

TEST_CASE("the player's own lines are dropped")
{
	CHECK_FALSE(IsWorthKeeping(Line(kSceneSubtype, 1, false, "Skyrim.esm", true)));
}

TEST_CASE("Mantella's lines are dropped, in any case")
{
	CHECK_FALSE(IsWorthKeeping(Line(kSceneSubtype, 1, false, "Mantella.esp")));
	CHECK_FALSE(IsWorthKeeping(Line(0, 0, true, "MANTELLA.ESP")));  // An answer to Mantella's menu line
	CHECK(IsWorthKeeping(Line(kSceneSubtype, 1, false, "MantellaExtra.esp")));
}

TEST_CASE("menu talk is kept, and not without the menu")
{
	CHECK(IsWorthKeeping(Line(0, 0, true)));
	CHECK_FALSE(IsWorthKeeping(Line(0, 0, false)));
}

TEST_CASE("the greeting when the dialogue menu opens is dropped, also when the speaker is in a scene")
{
	CHECK_FALSE(IsWorthKeeping(Line(kHelloSubtype, 0, true)));
	CHECK_FALSE(IsWorthKeeping(Line(kHelloSubtype, 0x000ABCDE, true)));  // Talking to Mikael while his scene still runs
	CHECK(IsWorthKeeping(Line(kSceneSubtype, 0x000ABCDE, true)));        // A scene line said in the dialogue menu
	CHECK(IsWorthKeeping(Line(kHelloSubtype, 0x000ABCDE, false)));       // A greeting inside a scene, menu closed
}

TEST_CASE("a line reported again is let through once while it is the speaker's latest")
{
	RepeatGuard guard;
	CHECK(guard.IsNew(1, 100));
	CHECK_FALSE(guard.IsNew(1, 100));  // The same line, once per response
	CHECK(guard.IsNew(2, 100));        // Someone else
	CHECK(guard.IsNew(1, 101));
	CHECK(guard.IsNew(1, 100));  // Said again later
	guard.Clear();
	CHECK(guard.IsNew(1, 100));
}
