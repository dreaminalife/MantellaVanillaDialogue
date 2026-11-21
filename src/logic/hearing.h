#pragma once

#include "logic/heard_line.h"
#include "logic/settings.h"

#include <optional>
#include <vector>

namespace HeardLines
{
	// Who is around, read from the game
	class IActorScan
	{
	public:
		virtual ~IActorScan() = default;
		// The loaded actors near the player, without the player
		virtual std::vector<NearbyActor> Nearby() = 0;
		virtual std::optional<Point>     PlayerPosition() = 0;
	};

	struct Hearing
	{
		std::vector<Person> heardBy;  // Never the player
		bool                playerThere = false;
	};

	// Works out who heard a line
	class IHearingCheck
	{
	public:
		virtual ~IHearingCheck() = default;
		virtual Hearing Check(const Person& a_speaker, const Point& a_speakerAt, const std::optional<Person>& a_to, IActorScan& a_scan,
			const Settings& a_settings) = 0;
	};

	// The speaker, the person spoken to, and, with Nearby NPCs Overhear on, everyone alive and enabled within Hearing Distance
	// of the speaker. Nearby NPCs Overhear off: nobody else, and the actors around aren't even looked at
	class DistanceHearingCheck : public IHearingCheck
	{
	public:
		Hearing Check(const Person& a_speaker, const Point& a_speakerAt, const std::optional<Person>& a_to, IActorScan& a_scan,
			const Settings& a_settings) override;
	};

	[[nodiscard]] bool IsWithin(const Point& a_from, const Point& a_to, float a_distance);
}
