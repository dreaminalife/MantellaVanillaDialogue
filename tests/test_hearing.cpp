// Who heard a line
#include "logic/hearing.h"

#include <doctest/doctest.h>

#include <algorithm>

using namespace HeardLines;

namespace
{
	const Person kMikael{ 0x1A671, "Mikael" };
	const Person kCarlotta{ 0x1A672, "Carlotta" };
	const Person kSaadia{ 0xD7505, "Saadia" };
	const Person kHulda{ 0x1A66E, "Hulda" };
	const Person kPlayer{ kPlayerRef, "Kit" };

	class FakeScan : public IActorScan
	{
	public:
		std::vector<NearbyActor> actors;
		std::optional<Point>     player;
		int                      scans = 0;

		std::vector<NearbyActor> Nearby() override
		{
			++scans;
			return actors;
		}
		std::optional<Point> PlayerPosition() override { return player; }
	};

	Settings With(float a_distance, bool a_overhear = true)
	{
		return { true, a_overhear, a_distance, 200 };
	}

	std::vector<std::string> Names(const Hearing& a_hearing)
	{
		std::vector<std::string> names;
		for (const auto& person : a_hearing.heardBy) {
			names.push_back(person.name);
		}
		return names;
	}
}

TEST_CASE("everyone within Hearing Distance hears it, exactly at the distance too")
{
	FakeScan scan;
	scan.actors = { { kSaadia, { 100, 0, 0 } }, { kHulda, { 100.5f, 0, 0 } } };
	DistanceHearingCheck check;
	const auto           hearing = check.Check(kMikael, { 0, 0, 0 }, kCarlotta, scan, With(100));
	CHECK(Names(hearing) == std::vector<std::string>{ "Mikael", "Carlotta", "Saadia" });
}

TEST_CASE("height counts toward the distance")
{
	FakeScan scan;
	scan.actors = { { kHulda, { 60, 0, 80 } } };  // 100 away
	DistanceHearingCheck check;
	CHECK(Names(check.Check(kMikael, {}, std::nullopt, scan, With(99))) == std::vector<std::string>{ "Mikael" });
	CHECK(Names(check.Check(kMikael, {}, std::nullopt, scan, With(100))) == std::vector<std::string>{ "Mikael", "Hulda" });
}

TEST_CASE("the dead and the disabled hear nothing")
{
	FakeScan scan;
	scan.actors = { { kSaadia, { 1, 0, 0 }, false, true }, { kHulda, { 1, 0, 0 }, true, false } };
	DistanceHearingCheck check;
	CHECK(Names(check.Check(kMikael, {}, std::nullopt, scan, With(100))) == std::vector<std::string>{ "Mikael" });
}

TEST_CASE("the speaker and the person spoken to always hear it, wherever they are")
{
	FakeScan scan;
	scan.actors = { { kCarlotta, { 5000, 0, 0 } } };
	DistanceHearingCheck check;
	CHECK(Names(check.Check(kMikael, {}, kCarlotta, scan, With(100))) == std::vector<std::string>{ "Mikael", "Carlotta" });
}

TEST_CASE("the player is never among who heard it, also as the person spoken to")
{
	FakeScan scan;
	scan.actors = { { kPlayer, { 1, 0, 0 } } };
	DistanceHearingCheck check;
	CHECK(Names(check.Check(kHulda, {}, kPlayer, scan, With(100))) == std::vector<std::string>{ "Hulda" });
}

TEST_CASE("whether the player was there")
{
	FakeScan             scan;
	DistanceHearingCheck check;
	scan.player = Point{ 50, 0, 0 };
	CHECK(check.Check(kMikael, {}, std::nullopt, scan, With(100)).playerThere);
	scan.player = Point{ 150, 0, 0 };
	CHECK_FALSE(check.Check(kMikael, {}, std::nullopt, scan, With(100)).playerThere);
	scan.player.reset();
	CHECK_FALSE(check.Check(kMikael, {}, std::nullopt, scan, With(100)).playerThere);
}

TEST_CASE("Nearby NPCs Overhear off: only the speaker and the person spoken to, without looking around")
{
	FakeScan scan;
	scan.actors = { { kSaadia, { 1, 0, 0 } } };
	scan.player = Point{ 1, 0, 0 };
	DistanceHearingCheck check;
	const auto           hearing = check.Check(kMikael, {}, kCarlotta, scan, With(100, false));
	CHECK(Names(hearing) == std::vector<std::string>{ "Mikael", "Carlotta" });
	CHECK(scan.scans == 0);
	CHECK(hearing.playerThere);
}

TEST_CASE("nobody is listed twice")
{
	FakeScan scan;
	scan.actors = { { kCarlotta, { 1, 0, 0 } }, { kMikael, { 0, 0, 0 } } };
	DistanceHearingCheck check;
	CHECK(Names(check.Check(kMikael, {}, kCarlotta, scan, With(100))) == std::vector<std::string>{ "Mikael", "Carlotta" });
}
