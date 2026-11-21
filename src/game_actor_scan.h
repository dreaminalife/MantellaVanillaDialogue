#pragma once

#include "logic/hearing.h"

namespace HeardLines
{
	// The loaded actors near the player (the game's high-detail actors). Only on the main thread
	class GameActorScan : public IActorScan
	{
	public:
		std::vector<NearbyActor> Nearby() override;
		std::optional<Point>     PlayerPosition() override;
	};
}
