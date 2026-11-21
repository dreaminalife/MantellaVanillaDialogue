// The settings the game add-on sends
#include "logic/settings.h"

#include <doctest/doctest.h>

using namespace HeardLines;

TEST_CASE("nothing is caught before settings arrive")
{
	SettingsHolder settings;
	CHECK_FALSE(settings.Get().has_value());
	CHECK_FALSE(settings.IsCatching());
}

TEST_CASE("settings arrive, and Vanilla Dialogues off catches nothing")
{
	SettingsHolder settings;
	CHECK(settings.Set(true, false, 1420.4f, 200));
	CHECK(settings.IsCatching());
	CHECK(settings.Get()->hearingDistance == doctest::Approx(1420.4f));
	CHECK_FALSE(settings.Get()->nearbyOverhear);
	CHECK(settings.Set(false, true, 1420.4f, 200));
	CHECK_FALSE(settings.IsCatching());
}

TEST_CASE("zeros are refused and change nothing")
{
	SettingsHolder settings;
	CHECK_FALSE(settings.Set(true, true, 0, 200));
	CHECK_FALSE(settings.Set(true, true, 100, 0));
	CHECK_FALSE(settings.Set(true, true, -5, 200));
	CHECK_FALSE(settings.Get().has_value());
	CHECK(settings.Set(true, true, 100, 10));
	CHECK_FALSE(settings.Set(false, true, 0, 10));
	CHECK(settings.IsCatching());
}
