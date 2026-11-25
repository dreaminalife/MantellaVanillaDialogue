#include "game_line_reader.h"

#include <array>
#include <ctime>

namespace HeardLines
{
	namespace
	{
		// The words of every response, as the game builds them when the line is spoken. Read from the line itself, not the subtitle.
		// The game allocates the responses on its own heap and frees them with its own code, which CommonLib doesn't expose, so the
		// few bytes of each kept line's responses are left to the game
		std::string WordsOf(RE::TESTopicInfo* a_info, RE::TESObjectREFR* a_speaker)
		{
			if (!a_info->parentTopic) {
				return "";
			}
			auto        item = a_info->GetDialogueData(a_speaker);
			std::string words;
			for (const auto* response : item.responses) {
				const char* text = response ? response->text.c_str() : nullptr;
				if (text && text[0]) {
					words += words.empty() ? text : " "s + text;
				}
			}
			return words;
		}

		// In a two-person scene, the other person
		RE::TESObjectREFR* OtherSceneActor(RE::BGSScene* a_scene, RE::TESObjectREFR* a_speaker)
		{
			if (!a_scene || !a_scene->parentQuest) {
				return nullptr;
			}
			RE::TESObjectREFR* other = nullptr;
			int                count = 0;
			for (const auto aliasID : a_scene->actors) {
				const auto ref = a_scene->parentQuest->GetAliasedRef(aliasID).get();
				if (!ref || ref.get() == a_speaker) {
					continue;
				}
				if (ref.get() != other) {
					other = ref.get();
					++count;
				}
			}
			return count == 1 ? other : nullptr;
		}

		std::string PlaceOf(RE::TESObjectREFR* a_ref)
		{
			const auto* location = a_ref->GetCurrentLocation();
			const char* name = location ? location->GetName() : nullptr;
			if (name && name[0]) {
				return name;
			}
			const auto* cell = a_ref->GetParentCell();
			name = cell ? cell->GetName() : nullptr;
			return name ? name : "";
		}

		bool IsMenuTalk(RE::TESObjectREFR* a_speaker)
		{
			auto*       ui = RE::UI::GetSingleton();
			const auto* topics = RE::MenuTopicManager::GetSingleton();
			return ui && topics && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) && topics->IsCurrentSpeaker(a_speaker->GetHandle());
		}

		// The option among these that leads to this line: the one for this very line, else one of its topic. Not a const list:
		// CommonLib's const iteration of it doesn't compile
		const RE::MenuTopicManager::Dialogue* OptionFor(RE::BSSimpleList<RE::MenuTopicManager::Dialogue*>* a_options, const RE::TESTopicInfo* a_info)
		{
			if (!a_options) {
				return nullptr;
			}
			const RE::MenuTopicManager::Dialogue* sameTopic = nullptr;
			for (const auto* option : *a_options) {
				if (!option) {
					continue;
				}
				if (option->parentTopicInfo == a_info) {
					return option;
				}
				if (!sameTopic && a_info->parentTopic && option->parentTopic == a_info->parentTopic) {
					sameTopic = option;
				}
			}
			return sameTopic;
		}

		// What the player picked for the NPC to answer with this line, as the menu showed it. Not the game's last pick: when the answer
		// starts, that still holds the pick before (the greeting for the first answer). Only read while the menu is open with this speaker:
		// once it closes, the game may free the options.
		// Empty when no option on screen leads to the line: the NPC speaks unprompted (a forced greeting, a line that carries on by
		// itself). Never the topic's own name: for those it is a label the player never sees, like TG01Hellos or
		// (Invisible Continues Linked to Self)
		std::string PickFor(const RE::TESTopicInfo* a_info)
		{
			auto*       topics = RE::MenuTopicManager::GetSingleton();
			const auto* option = OptionFor(topics->dialogueList, a_info);
			// A pick that opened more options may have put its list under the new one
			for (auto* node = topics->dialogueListStack; !option && node; node = node->next) {
				option = OptionFor(node->item, a_info);
			}
			return option && option->topicText.c_str() ? option->topicText.c_str() : "";
		}

		// Background chatter check (temporary)
		std::string_view QuestTypeName(RE::QUEST_DATA::Type a_type)
		{
			static constexpr std::array<std::string_view, 12> names{ "None", "Main Quest", "Mages Guild", "Thieves Guild",
				"Dark Brotherhood", "Companions", "Misc", "Daedric", "Side Quest", "Civil War", "Dawnguard", "Dragonborn" };
			const auto index = static_cast<std::size_t>(a_type);
			return index < names.size() ? names[index] : "?";
		}

		std::string QuestStartName(RE::QuestEvent a_event)
		{
			switch (a_event) {
			case RE::QuestEvent::kNone:
				return "no story event";
			case RE::QuestEvent::kActorDialogue:
				return "two NPCs meeting (ADIA)";
			case RE::QuestEvent::kActorHello:
				return "an NPC greeting (AHEL)";
			case RE::QuestEvent::kChangeLocation:
				return "a location change (CLOC)";
			case RE::QuestEvent::kScript:
				return "a script (SCPT)";
			default:
				return std::format("story event {}", static_cast<int>(a_event));
			}
		}

		std::string DescribeQuest(const RE::TESQuest* a_quest)
		{
			if (!a_quest) {
				return "none";
			}
			const auto* file = a_quest->GetFile(0);
			const char* editorId = a_quest->GetFormEditorID();
			std::string flags;
			if (a_quest->data.flags.all(RE::QuestFlag::kStartsEnabled)) {
				flags += ", starts with the game";
			}
			if (a_quest->data.flags.all(RE::QuestFlag::kRunOnce)) {
				flags += ", runs once";
			}
			return std::format("{} {:08X} [{}], type {}, started by {}{}", editorId && editorId[0] ? editorId : "(no editor ID)",
				a_quest->GetFormID(), file ? file->GetFilename() : "?", QuestTypeName(a_quest->GetType()), QuestStartName(a_quest->eventID),
				flags);
		}
	}

	std::string GameLineReader::DescribeSource(RE::TESObjectREFR* a_speaker, RE::TESTopicInfo* a_info)
	{
		const auto* scene = a_speaker->GetCurrentScene();
		std::string text = "scene quest: ";
		if (scene) {
			const char* sceneId = scene->GetFormEditorID();
			text += DescribeQuest(scene->parentQuest);
			text += std::format(" | scene {:08X}{}{}{}", scene->GetFormID(), sceneId && sceneId[0] ? " "s + sceneId : "",
				scene->flags.all(RE::BGSScene::Flag::kBeginOnQuestStart) ? ", begins on quest start" : "",
				scene->flags.all(RE::BGSScene::Flag::kRepeatConditionsWhileTrue) ? ", repeats while conditions hold" : "");
		} else {
			text += "no scene";
		}
		const auto* lineQuest = a_info->parentTopic ? a_info->parentTopic->ownerQuest : nullptr;
		if (!scene || lineQuest != scene->parentQuest) {
			text += " | line's quest: " + DescribeQuest(lineQuest);
		}
		return text;
	}

	LineFacts GameLineReader::ReadFacts(RE::TESObjectREFR* a_speaker, RE::TESTopicInfo* a_info)
	{
		LineFacts facts;
		facts.speakerIsPlayer = a_speaker->IsPlayerRef();
		const auto* file = a_info->GetFile(0);
		facts.fileName = file ? std::string(file->GetFilename()) : "";
		facts.subtype = a_info->parentTopic ? static_cast<int>(a_info->parentTopic->data.subtype.underlying()) : -1;
		const auto* scene = a_speaker->GetCurrentScene();
		facts.sceneId = scene ? scene->GetFormID() : 0;
		facts.menuTalk = !facts.speakerIsPlayer && IsMenuTalk(a_speaker);
		facts.speakerLoaded = a_speaker->Is3DLoaded();
		return facts;
	}

	std::optional<HeardLine> GameLineReader::ReadLine(RE::TESObjectREFR* a_speaker, RE::TESTopicInfo* a_info, const LineFacts& a_facts)
	{
		HeardLine line;
		line.words = WordsOf(a_info, a_speaker);
		if (line.words.empty()) {
			return std::nullopt;
		}
		line.speaker = PersonOf(a_speaker);
		line.menu = a_facts.menuTalk;
		line.scene = a_facts.menuTalk ? 0 : a_facts.sceneId;
		if (a_facts.menuTalk) {
			line.to = PersonOf(RE::PlayerCharacter::GetSingleton());
			line.asked = PickFor(a_info);
		} else {
			RE::TESObjectREFR* to = nullptr;
			if (auto* actor = a_speaker->As<RE::Actor>()) {
				to = actor->GetActorRuntimeData().dialogueItemTarget.get().get();
			}
			if (!to) {
				to = OtherSceneActor(a_speaker->GetCurrentScene(), a_speaker);
			}
			if (to && to != a_speaker) {
				line.to = PersonOf(to);
			}
		}
		line.place = PlaceOf(a_speaker);
		line.time = Now();
		line.realTime = static_cast<std::int64_t>(std::time(nullptr));
		return line;
	}

	Person GameLineReader::PersonOf(RE::TESObjectREFR* a_ref)
	{
		const char* name = a_ref ? a_ref->GetDisplayFullName() : nullptr;
		return { a_ref ? a_ref->GetFormID() : 0, name ? name : "" };
	}

	Point GameLineReader::PointOf(const RE::NiPoint3& a_point)
	{
		return { a_point.x, a_point.y, a_point.z };
	}

	GameTime GameLineReader::Now()
	{
		const auto* calendar = RE::Calendar::GetSingleton();
		return calendar ? GameTime{ calendar->GetDaysPassed(), calendar->GetHour() } : GameTime{};
	}

	bool GameLineReader::IsTimeKnown()
	{
		const auto* calendar = RE::Calendar::GetSingleton();
		return calendar && calendar->gameDaysPassed;
	}
}
