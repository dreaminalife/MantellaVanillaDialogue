// MantellaHeardLines, test build: writes every voiced line Skyrim reports to the log, so we can see
// what the game gives us before the real plugin is written. Nothing is kept or handed to Mantella.
//
// The log is Documents\My Games\Skyrim Special Edition\SKSE\MantellaHeardLines.log

namespace
{
	std::thread::id g_mainThread;

	std::string Describe(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return "none";
		}
		const char* name = a_ref->GetDisplayFullName();
		return std::format("{} [{:08X}]", name ? name : "", a_ref->GetFormID());
	}

	std::string Describe(const RE::ObjectRefHandle& a_handle)
	{
		const auto ref = a_handle.get();
		return Describe(ref.get());
	}

	std::string FileName(const RE::TESForm* a_form)
	{
		const auto* file = a_form ? a_form->GetFile(0) : nullptr;
		return file ? std::string(file->GetFilename()) : "none";
	}

	std::string Place(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return "none";
		}
		const auto* location = a_ref->GetCurrentLocation();
		const auto* cell = a_ref->GetParentCell();
		return std::format("location='{}' cell='{}'", location ? location->GetName() : "", cell ? cell->GetName() : "");
	}

	// The words, as the game builds them when the line is spoken
	std::string WordsFromDialogueData(RE::TESTopicInfo* a_info, RE::TESObjectREFR* a_speaker)
	{
		if (!a_info->parentTopic) {
			return "(no topic)";
		}
		auto        item = a_info->GetDialogueData(a_speaker);
		std::string words;
		for (const auto* response : item.responses) {
			if (response) {
				words += std::format("\"{}\" ", response->text.c_str());
			}
		}
		return words.empty() ? "(none)" : words;
	}

	std::string SceneActors(const RE::BGSScene* a_scene)
	{
		if (!a_scene || !a_scene->parentQuest) {
			return "";
		}
		std::string actors;
		for (const auto aliasID : a_scene->actors) {
			actors += std::format("alias {} = {}; ", aliasID, Describe(a_scene->parentQuest->GetAliasedRef(aliasID)));
		}
		return actors;
	}

	class TopicLogger : public RE::BSTEventSink<RE::TESTopicInfoEvent>
	{
	public:
		static TopicLogger* GetSingleton()
		{
			static TopicLogger singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const bool onMainThread = std::this_thread::get_id() == g_mainThread;
			auto*      speaker = a_event->speakerRef.get();
			const bool begin = a_event->type == RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin;
			if (!begin) {
				logs::info("END   info={:08X} stage={} speaker={} main_thread={}", a_event->topicInfoFormID, a_event->stage, Describe(speaker), onMainThread);
				return RE::BSEventNotifyControl::kContinue;
			}

			auto*       info = RE::TESForm::LookupByID<RE::TESTopicInfo>(a_event->topicInfoFormID);
			const auto* topic = info ? info->parentTopic : nullptr;
			const auto* quest = topic ? topic->ownerQuest : nullptr;
			logs::info("BEGIN info={:08X} stage={} file={} main_thread={}", a_event->topicInfoFormID, a_event->stage, FileName(info), onMainThread);
			logs::info("  topic={:08X} type={} subtype={} quest={}", topic ? topic->GetFormID() : 0, topic ? static_cast<int>(topic->data.type.underlying()) : -1,
				topic ? static_cast<int>(topic->data.subtype.underlying()) : -1, quest ? quest->GetFormEditorID() : "none");

			auto* actor = speaker ? speaker->As<RE::Actor>() : nullptr;
			logs::info("  speaker={} is_player={} target={}", Describe(speaker), speaker && speaker->IsPlayerRef(),
				actor ? Describe(actor->GetActorRuntimeData().dialogueItemTarget) : "(not an actor)");

			const auto* scene = speaker ? speaker->GetCurrentScene() : nullptr;
			logs::info("  speaker_scene={:08X} scene_quest={} {}", scene ? scene->GetFormID() : 0,
				scene && scene->parentQuest ? scene->parentQuest->GetFormEditorID() : "none", SceneActors(scene));

			auto*       ui = RE::UI::GetSingleton();
			const bool  menuOpen = ui && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
			const auto* topics = RE::MenuTopicManager::GetSingleton();
			const bool  menuSpeaker = topics && speaker && topics->IsCurrentSpeaker(speaker->GetHandle());
			// Only while the menu is open with this speaker: once it closes, the game may free the pick
			const auto* pick = menuOpen && menuSpeaker ? topics->lastSelectedDialogue : nullptr;
			logs::info("  dialogue_menu_open={} menu_speaker={} pick=\"{}\"", menuOpen, menuSpeaker, pick ? pick->topicText.c_str() : "");

			if (info) {
				logs::info("  words: {}", WordsFromDialogueData(info, speaker));
			}

			const auto* calendar = RE::Calendar::GetSingleton();
			logs::info("  day={:.4f} hour={:.2f} global_present={} {}", calendar ? calendar->GetDaysPassed() : -1.0f, calendar ? calendar->GetHour() : -1.0f,
				calendar && calendar->gameDaysPassed, Place(speaker));
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	void OnDataLoaded()
	{
		g_mainThread = std::this_thread::get_id();
		if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
			holder->AddEventSink<RE::TESTopicInfoEvent>(TopicLogger::GetSingleton());
			logs::info("Listening for spoken lines");
		}
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
		if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			OnDataLoaded();
		}
	});

	logs::info("MantellaHeardLines loaded (test build: logs only)");
	return true;
}
