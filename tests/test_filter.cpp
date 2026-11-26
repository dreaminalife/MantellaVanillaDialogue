// Which lines are kept: scene lines and menu talk, never the player's, Mantella's or chatter
#include "logic/line_filter.h"

#include <doctest/doctest.h>

using namespace HeardLines;

namespace
{
	LineFacts Line(int a_subtype, std::uint32_t a_scene = 0, bool a_menu = false, std::string a_file = "Skyrim.esm", bool a_player = false,
		bool a_loaded = true, bool a_topicInScene = false)
	{
		return { a_player, std::move(a_file), a_subtype, a_scene, a_menu, a_loaded, a_topicInScene };
	}

	// A line said by someone in a scene, one the scene has them say or not
	LineFacts InScene(int a_subtype, bool a_sceneHasThemSayIt)
	{
		return Line(a_subtype, 0x000ABCDE, false, "Skyrim.esm", false, true, a_sceneHasThemSayIt);
	}
}

TEST_CASE("a speaker outside the loaded area is dropped, in a scene or in the dialogue menu")
{
	CHECK_FALSE(IsWorthKeeping(Line(kSceneSubtype, 0x000ABCDE, false, "Skyrim.esm", false, false)));
	CHECK_FALSE(IsWorthKeeping(Line(0, 0, true, "Skyrim.esm", false, false)));
	CHECK(IsWorthKeeping(Line(kSceneSubtype, 0x000ABCDE, false, "Skyrim.esm", false, true)));
}

TEST_CASE("scene lines are kept")
{
	CHECK(IsWorthKeeping(Line(kSceneSubtype)));
}

TEST_CASE("a line the scene has them say is kept, whatever Skyrim files it under")
{
	CHECK(IsWorthKeeping(InScene(19, true)));  // Five Dragonborn scenes use lines Skyrim doesn't file as scene lines
	CHECK(IsWorthKeeping(InScene(94, true)));
}

TEST_CASE("a greeting, idle remark or combat shout from someone in a scene is dropped, also from the scene's own quest")
{
	for (const int subtype : { 79 /* greeting */, 94 /* idle */, 26, 40, 64 /* combat */, 101 /* grunt */, 0 /* a quest's own line */ }) {
		CAPTURE(subtype);
		CHECK_FALSE(IsWorthKeeping(InScene(subtype, false)));
	}
}

TEST_CASE("a scene line is kept even when the speaker isn't in the scene any more, or the check didn't run")
{
	CHECK(IsWorthKeeping(Line(kSceneSubtype)));
	CHECK(IsWorthKeeping(InScene(kSceneSubtype, false)));
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
	CHECK(IsWorthKeeping(InScene(kHelloSubtype, true)));                 // A greeting the scene has them say, menu closed
	CHECK_FALSE(IsWorthKeeping(InScene(kHelloSubtype, false)));          // Greeting a passer-by while the scene runs
}

TEST_CASE("the goodbye when the dialogue menu closes is dropped")
{
	CHECK_FALSE(IsWorthKeeping(Line(kGoodbyeSubtype, 0, true)));
	CHECK_FALSE(IsWorthKeeping(Line(kGoodbyeSubtype, 0x000ABCDE, true)));
	CHECK(IsWorthKeeping(InScene(kGoodbyeSubtype, true)));  // The scene has them say it, menu closed
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
