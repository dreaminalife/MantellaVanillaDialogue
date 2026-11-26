#include "logic/line_store.h"

#include <algorithm>

#include <nlohmann/json.hpp>

namespace HeardLines
{
	namespace
	{
		using json = nlohmann::json;

		json ToJson(const Person& a_person)
		{
			return { { "id", a_person.id }, { "name", a_person.name } };
		}

		std::optional<Person> PersonFrom(const json& a_data)
		{
			if (!a_data.is_object() || !a_data.contains("id") || !a_data["id"].is_number_unsigned()) {
				return std::nullopt;
			}
			return Person{ a_data["id"].get<std::uint32_t>(), a_data.value("name", "") };
		}

		// The hand-over format (see Mantella's src/heard_lines/heard_line.py), also used in the save
		json ToJson(const HeardLine& a_line)
		{
			json heardBy = json::array();
			for (const auto& person : a_line.heardBy) {
				heardBy.push_back(ToJson(person));
			}
			json result = { { "day", a_line.time.day }, { "hour", a_line.time.hour }, { "time", a_line.realTime }, { "place", a_line.place },
				{ "scene", a_line.scene }, { "menu", a_line.menu }, { "speaker", ToJson(a_line.speaker) }, { "words", a_line.words },
				{ "asked", a_line.asked }, { "heard_by", heardBy }, { "player_there", a_line.playerThere } };
			if (a_line.to) {
				result["to"] = ToJson(*a_line.to);
			}
			if (a_line.quest) {
				result["quest"] = { { "id", a_line.quest->id }, { "journal", a_line.quest->inJournal }, { "runs_once", a_line.quest->runsOnce },
					{ "npcs_meet", a_line.quest->startsWhenNpcsMeet }, { "follows_quest", a_line.quest->followsQuest } };
			}
			if (!a_line.sceneKey.empty()) {
				result["scene_key"] = a_line.sceneKey;
			}
			return result;
		}

		std::optional<SceneQuest> QuestFrom(const json& a_data)
		{
			if (!a_data.is_object()) {
				return std::nullopt;
			}
			return SceneQuest{ a_data.value("id", ""), a_data.value("journal", false), a_data.value("runs_once", false),
				a_data.value("npcs_meet", false), a_data.value("follows_quest", false) };
		}

		std::optional<HeardLine> LineFrom(const json& a_data)
		{
			if (!a_data.is_object()) {
				return std::nullopt;
			}
			const auto speaker = PersonFrom(a_data.value("speaker", json()));
			if (!speaker || !a_data.value("day", json()).is_number() || !a_data.value("hour", json()).is_number()) {
				return std::nullopt;
			}
			HeardLine line;
			line.time = { a_data["day"].get<double>(), a_data["hour"].get<double>() };
			line.realTime = a_data.value("time", std::int64_t{ 0 });
			line.place = a_data.value("place", "");
			line.scene = a_data.value("scene", std::uint32_t{ 0 });
			line.quest = QuestFrom(a_data.value("quest", json()));  // Missing in saves from before it was sent
			line.sceneKey = a_data.value("scene_key", "");
			line.menu = a_data.value("menu", false);
			line.speaker = *speaker;
			line.to = PersonFrom(a_data.value("to", json()));
			line.words = a_data.value("words", "");
			line.asked = a_data.value("asked", "");
			for (const auto& person : a_data.value("heard_by", json::array())) {
				if (auto listener = PersonFrom(person)) {
					line.heardBy.push_back(*listener);
				}
			}
			line.playerThere = a_data.value("player_there", false);
			return line;
		}

		// Names and words in the game's own code page come out as replacement characters, instead of failing the whole hand-over
		std::string Dump(const json& a_data)
		{
			return a_data.dump(-1, ' ', false, json::error_handler_t::replace);
		}
	}

	void CoSaveLineStore::Add(HeardLine a_line, std::size_t a_mostWaiting)
	{
		std::scoped_lock lock(_lock);
		_lines.push_back(std::move(a_line));
		while (_lines.size() > a_mostWaiting) {
			// Chatter goes first, so a preacher talking for hours can't push out a story scene before the next hand-over
			const auto chatter = std::ranges::find_if(_lines, [](const HeardLine& line) { return line.quest && line.quest->LooksLikeChatter(); });
			_lines.erase(chatter != _lines.end() ? chatter : _lines.begin());
		}
	}

	std::string CoSaveLineStore::TakeAllAsJson(const std::optional<GameTime>& a_now, const std::string& a_player)
	{
		json lines = json::array();
		{
			std::scoped_lock lock(_lock);
			for (const auto& line : _lines) {
				lines.push_back(ToJson(line));
			}
			_lines.clear();
		}
		json result = { { "player", a_player }, { "lines", lines } };
		if (a_now) {
			result["now"] = { { "day", a_now->day }, { "hour", a_now->hour } };
		}
		return Dump(result);
	}

	void CoSaveLineStore::Save(ISaveRecord& a_record)
	{
		json lines = json::array();
		{
			std::scoped_lock lock(_lock);
			for (const auto& line : _lines) {
				lines.push_back(ToJson(line));
			}
		}
		a_record.Write(kVersion, Dump({ { "lines", lines } }));
	}

	bool CoSaveLineStore::Load(ISaveRecord& a_record)
	{
		Clear();
		const auto record = a_record.Read();
		if (!record || record->first != kVersion) {
			return false;
		}
		const auto data = json::parse(record->second, nullptr, false);
		if (data.is_discarded() || !data.is_object() || !data.value("lines", json()).is_array()) {
			return false;
		}
		std::scoped_lock lock(_lock);
		for (const auto& item : data["lines"]) {
			if (auto line = LineFrom(item)) {
				_lines.push_back(std::move(*line));
			}
		}
		return true;
	}

	void CoSaveLineStore::Clear()
	{
		std::scoped_lock lock(_lock);
		_lines.clear();
	}

	std::size_t CoSaveLineStore::Count()
	{
		std::scoped_lock lock(_lock);
		return _lines.size();
	}
}
