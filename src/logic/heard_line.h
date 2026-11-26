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
		bool          topicInScene = false;  // The line is one the speaker's scene has them say, not a greeting, idle remark or combat
		                                     // shout said while the scene runs
	};

	// A nearby actor, as the hearing check sees them
	struct NearbyActor
	{
		Person person;
		Point  position;
		bool   alive = true;
		bool   enabled = true;
	};

	inline constexpr int kQuestTypeNone = 0;  // QUEST_DATA::Type::kNone
	inline constexpr int kQuestTypeMisc = 6;  // QUEST_DATA::Type::kMiscellaneous

	// A quest the player follows: one the journal shows. Filed under a journal category, or with objectives (some real quests, like
	// The Heart of Dibella, have no category). The journal only lists Miscellaneous quests by their objectives, so one without any isn't
	// shown: Bethesda filed some small talk that way (a tavern server and a patron, a child playing invisible)
	[[nodiscard]] constexpr bool IsJournalQuest(int a_type, bool a_hasObjectives)
	{
		return (a_type != kQuestTypeNone && a_type != kQuestTypeMisc) || a_hasObjectives;
	}

	// The quest a scene line belongs to, so Mantella can tell background chatter from story scenes
	struct SceneQuest
	{
		std::string id;  // The quest's editor ID, such as DialogueWhiterunHeimskrPreachScene
		bool        inJournal = false;  // Filed under a journal category, or has journal objectives: a quest the player can follow
		bool        runsOnce = false;  // Run Once ticked: it can't start again after it ends
		bool        startsWhenNpcsMeet = false;  // Started by the story event for two NPCs meeting (ADIA)
		bool        followsQuest = false;  // The quest or the scene checks how far a quest the player follows has got, like Enmon and Mena
		                                   // missing their daughter only after The Heart of Dibella

		// Small talk Skyrim starts when two NPCs meet, and never part of a quest the player follows, one that runs once or one that follows
		// a quest's progress. Mantella decides with its own switch and lists; this only picks which waiting lines go first when there are
		// too many
		[[nodiscard]] bool LooksLikeChatter() const { return startsWhenNpcsMeet && !inJournal && !runsOnce && !followsQuest; }

		bool operator==(const SceneQuest&) const = default;
	};

	// One caught line, waiting for the game add-on to hand it to Mantella
	struct HeardLine
	{
		GameTime                  time;
		std::int64_t              realTime = 0;  // Epoch seconds
		std::string               place;
		std::uint32_t             scene = 0;
		std::optional<SceneQuest> quest;  // Scene lines only: the scene's quest, else the line's own. Missing for menu talk
		std::string               sceneKey;  // Scene lines only: the scene as <file>:<6-hex form ID in that file>, such as Skyrim.esm:092A31,
		                                     // for scene entries in Mantella's lists. Empty for menu talk and outside a scene
		bool                      menu = false;  // Menu talk: the NPC answering the player's pick
		Person                    speaker;
		std::optional<Person>     to;
		std::string               words;
		std::string               asked;  // Menu talk: the player's pick. Empty when the NPC speaks unprompted (no option on screen led to it)
		std::vector<Person>       heardBy;  // Never the player
		bool                      playerThere = false;
	};
}
