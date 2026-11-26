#include "game_line_reader.h"

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

		bool IsInJournal(const RE::TESQuest* a_quest)
		{
			return IsJournalQuest(static_cast<int>(a_quest->GetType()), !a_quest->objectives.empty());
		}

		// One of the conditions checks how far another quest the player follows has got: its stage, whether it runs or is done
		bool ChecksQuestProgress(const RE::TESCondition& a_conditions, const RE::TESQuest* a_own)
		{
			using Function = RE::FUNCTION_DATA::FunctionID;
			for (const auto* item = a_conditions.head; item; item = item->next) {
				const auto function = item->data.functionData.function.get();
				if (function != Function::kGetStage && function != Function::kGetStageDone && function != Function::kGetQuestRunning &&
					function != Function::kGetQuestCompleted) {
					continue;
				}
				const auto* form = static_cast<const RE::TESForm*>(item->data.functionData.params[0]);
				const auto* quest = form && form->GetFormType() == RE::FormType::Quest ? static_cast<const RE::TESQuest*>(form) : nullptr;
				if (quest && quest != a_own && IsInJournal(quest)) {
					return true;
				}
			}
			return false;
		}

		// Small talk that follows a story: the quest (its dialogue or start conditions) or the scene checks another quest's progress.
		// The lines' own conditions aren't read: in some scenes only a few lines check, and the scene would be kept in pieces
		bool FollowsQuest(const RE::TESQuest* a_quest, const RE::BGSScene* a_scene)
		{
			return ChecksQuestProgress(a_quest->objConditions, a_quest) || ChecksQuestProgress(a_quest->storyManagerConditions, a_quest) ||
			       (a_scene && ChecksQuestProgress(a_scene->conditions, a_quest));
		}

		// The quest behind a scene line: the scene's, else the line's own (the speaker may have just left the scene). Read from the
		// quest's own settings, so it works the same for every mod
		std::optional<SceneQuest> QuestOf(RE::TESObjectREFR* a_speaker, const RE::TESTopicInfo* a_info)
		{
			const auto*        scene = a_speaker->GetCurrentScene();
			const RE::TESQuest* quest = scene ? scene->parentQuest : nullptr;
			if (!quest) {
				scene = nullptr;  // Its conditions say nothing about the line's own quest
				if (a_info->parentTopic) {
					quest = a_info->parentTopic->ownerQuest;
				}
			}
			if (!quest) {
				return std::nullopt;
			}
			const char* editorId = quest->GetFormEditorID();
			return SceneQuest{ editorId ? editorId : "", IsInJournal(quest), quest->data.flags.all(RE::QuestFlag::kRunOnce),
				quest->eventID == RE::QuestEvent::kActorDialogue, FollowsQuest(quest, scene) };
		}

		// The line is one of the scene's own dialogue actions: what the scene has this speaker (or another actor) say
		bool IsSceneLine(const RE::BGSScene* a_scene, const RE::TESTopicInfo* a_info)
		{
			if (!a_scene || !a_info->parentTopic) {
				return false;
			}
			for (const auto* action : a_scene->actions) {
				if (action && action->GetType() == RE::BGSSceneAction::Type::kDialogue &&
					static_cast<const RE::BGSSceneActionDialogue*>(action)->topic == a_info->parentTopic) {
					return true;
				}
			}
			return false;
		}

		// The scene as <file>:<6-hex form ID in that file>, the same in every load order. The file is the one that first added the scene
		std::string SceneKeyOf(const RE::BGSScene* a_scene)
		{
			const auto* file = a_scene ? a_scene->GetFile(0) : nullptr;
			if (!file) {
				return "";
			}
			return std::format("{}:{:06X}", file->GetFilename(), a_scene->GetLocalFormID());
		}
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
		// Only looked up when it decides: for a line said in a scene that Skyrim doesn't file as a scene line
		if (scene && !facts.menuTalk && facts.subtype != kSceneSubtype) {
			facts.topicInScene = IsSceneLine(scene, a_info);
		}
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
			line.quest = QuestOf(a_speaker, a_info);
			line.sceneKey = SceneKeyOf(a_speaker->GetCurrentScene());
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
