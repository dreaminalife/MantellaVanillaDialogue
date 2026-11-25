#pragma once

// Plain data about voiced lines, with no game types, so the logic and its tests build without CommonLib.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace HeardLines
{
	inline constexpr std::uint32_t kPlayerRef = 0x14;  // The player's reference
	inline constexpr int           kSceneSubtype = 14;  // DIALOGUE_DATA::Subtype::kScene
	inline constexpr int           kHelloSubtype = 79;  // DIALOGUE_DATA::Subtype::kHello
	inline constexpr int           kGoodbyeSubtype = 78;  // DIALOGUE_DATA::Subtype::kGoodBye

	struct Person
	{
		std::uint32_t id = 0;
		std::string   name;

		[[nodiscard]] bool IsPlayer() const { return id == kPlayerRef; }
		bool               operator==(const Person&) const = default;
	};

	struct Point
	{
		float x = 0;
		float y = 0;
		float z = 0;
	};

	struct GameTime
	{
		double day = 0;   // Days passed since the game started, the time of day as the fraction
		double hour = 0;  // 0 to 24
	};

	// What the game says about a line when it starts, before the words are read
	struct LineFacts
	{
		bool          speakerIsPlayer = false;
		std::string   fileName;     // The game file the line comes from
		int           subtype = -1;  // The kind of line Skyrim files it under
		std::uint32_t sceneId = 0;  // The scene the speaker is in now, 0 for none
		bool          menuTalk = false;  // The dialogue menu is open with this speaker
		bool          speakerLoaded = true;  // The speaker is in the area the game has loaded around the player
	};

	// A nearby actor, as the hearing check sees them
	struct NearbyActor
	{
		Person person;
		Point  position;
		bool   alive = true;
		bool   enabled = true;
	};

	// One caught line, waiting for the game add-on to hand it to Mantella
	struct HeardLine
	{
		GameTime              time;
		std::int64_t          realTime = 0;  // Epoch seconds
		std::string           place;
		std::uint32_t         scene = 0;
		bool                  menu = false;  // Menu talk: the NPC answering the player's pick
		Person                speaker;
		std::optional<Person> to;
		std::string           words;
		std::string           asked;  // Menu talk: the player's pick. Empty when the NPC speaks unprompted (no option on screen led to it)
		std::vector<Person>   heardBy;  // Never the player
		bool                  playerThere = false;
	};
}
