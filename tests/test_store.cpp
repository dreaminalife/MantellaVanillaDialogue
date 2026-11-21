// The waiting lines: the most-waiting limit, the hand-over JSON and the save
#include "logic/line_store.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using namespace HeardLines;
using json = nlohmann::json;

namespace
{
	HeardLine Line(std::string a_words, std::int64_t a_time = 1000)
	{
		HeardLine line;
		line.time = { 42.5, 12.0 };
		line.realTime = a_time;
		line.place = "The Bannered Mare";
		line.scene = 0x000ABCDE;
		line.speaker = { 0x1A671, "Mikael" };
		line.to = Person{ 0x1A672, "Carlotta" };
		line.words = std::move(a_words);
		line.heardBy = { { 0x1A671, "Mikael" }, { 0x1A672, "Carlotta" }, { 0xFE012ABC, "Saadia" } };
		line.playerThere = true;
		return line;
	}

	class MemoryRecord : public ISaveRecord
	{
	public:
		std::optional<std::pair<std::uint32_t, std::string>> saved;

		void Write(std::uint32_t a_version, const std::string& a_data) override { saved = std::make_pair(a_version, a_data); }
		std::optional<std::pair<std::uint32_t, std::string>> Read() override { return saved; }
	};

	std::vector<std::string> Words(const json& a_handOver)
	{
		std::vector<std::string> words;
		for (const auto& line : a_handOver["lines"]) {
			words.push_back(line["words"].get<std::string>());
		}
		return words;
	}
}

TEST_CASE("past the most waiting lines, the oldest are dropped")
{
	CoSaveLineStore store;
	for (int i = 0; i < 5; ++i) {
		store.Add(Line("Line " + std::to_string(i)), 3);
	}
	CHECK(store.Count() == 3);
	CHECK(Words(json::parse(store.TakeAllAsJson(std::nullopt, "Kit"))) == std::vector<std::string>{ "Line 2", "Line 3", "Line 4" });
}

TEST_CASE("taking the lines empties the store")
{
	CoSaveLineStore store;
	store.Add(Line("Once."), 200);
	CHECK(Words(json::parse(store.TakeAllAsJson(std::nullopt, "Kit"))).size() == 1);
	CHECK(store.Count() == 0);
	CHECK(json::parse(store.TakeAllAsJson(std::nullopt, "Kit"))["lines"].empty());
}

TEST_CASE("the JSON is the hand-over format Mantella reads")
{
	CoSaveLineStore store;
	store.Add(Line("Your beauty outshines the sun.", 1763600000), 200);
	const auto handOver = json::parse(store.TakeAllAsJson(GameTime{ 42.6, 14.4 }, "Kit"));
	CHECK(handOver["player"] == "Kit");
	CHECK(handOver["now"]["day"].get<double>() == doctest::Approx(42.6));
	CHECK(handOver["now"]["hour"].get<double>() == doctest::Approx(14.4));
	const auto& line = handOver["lines"][0];
	CHECK(line["day"].get<double>() == doctest::Approx(42.5));
	CHECK(line["hour"].get<double>() == doctest::Approx(12.0));
	CHECK(line["time"] == 1763600000);
	CHECK(line["place"] == "The Bannered Mare");
	CHECK(line["scene"] == 0x000ABCDE);
	CHECK(line["menu"] == false);
	CHECK(line["speaker"] == json{ { "id", 0x1A671 }, { "name", "Mikael" } });
	CHECK(line["to"] == json{ { "id", 0x1A672 }, { "name", "Carlotta" } });
	CHECK(line["words"] == "Your beauty outshines the sun.");
	CHECK(line["asked"] == "");
	CHECK(line["heard_by"].size() == 3);
	CHECK(line["heard_by"][2]["id"] == 0xFE012ABCu);  // Light plugin form ids stay unsigned
	CHECK(line["player_there"] == true);
}

TEST_CASE("without the game's time, now is left out; with nobody spoken to, to is left out")
{
	CoSaveLineStore store;
	auto            line = Line("To nobody.");
	line.to.reset();
	store.Add(line, 200);
	const auto handOver = json::parse(store.TakeAllAsJson(std::nullopt, "Kit"));
	CHECK_FALSE(handOver.contains("now"));
	CHECK_FALSE(handOver["lines"][0].contains("to"));
}

TEST_CASE("menu talk keeps the player's pick")
{
	CoSaveLineStore store;
	auto            line = Line("The Stormcloaks are losing.");
	line.menu = true;
	line.scene = 0;
	line.asked = "What news from the war?";
	store.Add(line, 200);
	const auto out = json::parse(store.TakeAllAsJson(std::nullopt, "Kit"))["lines"][0];
	CHECK(out["menu"] == true);
	CHECK(out["asked"] == "What news from the war?");
}

TEST_CASE("quotes, backslashes and other languages come through")
{
	const std::string words = "He said \"Ysmir\" \\ Dovahkiin, \xC3\xA7" "a va? \xE4\xBD\xA0\xE5\xA5\xBD";  // ça va? 你好 in UTF-8
	CoSaveLineStore   store;
	store.Add(Line(words), 200);
	CHECK(Words(json::parse(store.TakeAllAsJson(std::nullopt, "Kit")))[0] == words);
}

TEST_CASE("a name in the game's own code page doesn't break the hand-over")
{
	CoSaveLineStore store;
	auto            line = Line("Hello.");
	line.speaker.name = "J\xE9";  // Not UTF-8
	store.Add(line, 200);
	const auto handOver = json::parse(store.TakeAllAsJson(std::nullopt, "Kit"));
	CHECK(handOver["lines"][0]["words"] == "Hello.");
}

TEST_CASE("the waiting lines survive a save and load")
{
	CoSaveLineStore saved;
	saved.Add(Line("First."), 200);
	saved.Add(Line("Second."), 200);
	MemoryRecord record;
	saved.Save(record);
	CHECK(saved.Count() == 2);  // Saving keeps them waiting

	CoSaveLineStore loaded;
	CHECK(loaded.Load(record));
	const auto handOver = json::parse(loaded.TakeAllAsJson(std::nullopt, "Kit"));
	CHECK(Words(handOver) == std::vector<std::string>{ "First.", "Second." });
	CHECK(handOver["lines"][0]["heard_by"].size() == 3);
}

TEST_CASE("a record of an unknown version is skipped")
{
	CoSaveLineStore saved;
	saved.Add(Line("Old."), 200);
	MemoryRecord record;
	saved.Save(record);
	record.saved->first = CoSaveLineStore::kVersion + 1;
	CoSaveLineStore loaded;
	loaded.Add(Line("From before the load."), 200);
	CHECK_FALSE(loaded.Load(record));
	CHECK(loaded.Count() == 0);  // Loading a save never keeps lines from before it
}

TEST_CASE("a cut-off record is skipped without a crash")
{
	CoSaveLineStore saved;
	saved.Add(Line("Cut."), 200);
	MemoryRecord record;
	saved.Save(record);
	record.saved->second.resize(record.saved->second.size() / 2);
	CoSaveLineStore loaded;
	CHECK_FALSE(loaded.Load(record));
	CHECK(loaded.Count() == 0);
}

TEST_CASE("a save without the record loads nothing")
{
	MemoryRecord    record;
	CoSaveLineStore loaded;
	loaded.Add(Line("From another save."), 200);
	CHECK_FALSE(loaded.Load(record));
	CHECK(loaded.Count() == 0);
}

TEST_CASE("a new game clears the waiting lines")
{
	CoSaveLineStore store;
	store.Add(Line("Gone."), 200);
	store.Clear();
	CHECK(store.Count() == 0);
}
