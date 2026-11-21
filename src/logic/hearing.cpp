#include "logic/hearing.h"

#include <algorithm>

namespace HeardLines
{
	bool IsWithin(const Point& a_from, const Point& a_to, float a_distance)
	{
		const float dx = a_from.x - a_to.x;
		const float dy = a_from.y - a_to.y;
		const float dz = a_from.z - a_to.z;
		return dx * dx + dy * dy + dz * dz <= a_distance * a_distance;
	}

	Hearing DistanceHearingCheck::Check(const Person& a_speaker, const Point& a_speakerAt, const std::optional<Person>& a_to, IActorScan& a_scan,
		const Settings& a_settings)
	{
		Hearing result;
		const auto add = [&result](const Person& a_person) {
			const bool listed = std::any_of(result.heardBy.begin(), result.heardBy.end(), [&](const Person& p) { return p.id == a_person.id; });
			if (!a_person.IsPlayer() && !listed) {
				result.heardBy.push_back(a_person);
			}
		};
		add(a_speaker);
		if (a_to) {
			add(*a_to);
		}
		if (a_settings.nearbyOverhear) {
			for (const auto& actor : a_scan.Nearby()) {
				if (actor.alive && actor.enabled && IsWithin(a_speakerAt, actor.position, a_settings.hearingDistance)) {
					add(actor.person);
				}
			}
		}
		const auto player = a_scan.PlayerPosition();
		result.playerThere = player && IsWithin(a_speakerAt, *player, a_settings.hearingDistance);
		return result;
	}
}
