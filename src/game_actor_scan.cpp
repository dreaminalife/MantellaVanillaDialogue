#include "game_actor_scan.h"

#include "game_line_reader.h"

namespace HeardLines
{
	std::vector<NearbyActor> GameActorScan::Nearby()
	{
		std::vector<NearbyActor> result;
		auto*                    lists = RE::ProcessLists::GetSingleton();
		if (!lists) {
			return result;
		}
		for (auto& handle : lists->highActorHandles) {
			const auto actor = handle.get();
			if (!actor || actor->IsPlayerRef()) {
				continue;
			}
			result.push_back({ GameLineReader::PersonOf(actor.get()), GameLineReader::PointOf(actor->GetPosition()), !actor->IsDead(), !actor->IsDisabled() });
		}
		return result;
	}

	std::optional<Point> GameActorScan::PlayerPosition()
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return std::nullopt;
		}
		return GameLineReader::PointOf(player->GetPosition());
	}
}
