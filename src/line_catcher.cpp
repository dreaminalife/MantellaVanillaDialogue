#include "line_catcher.h"

#include "game_actor_scan.h"
#include "game_line_reader.h"

namespace HeardLines
{
	TopicLineCatcher::TopicLineCatcher(SettingsHolder& a_settings, ILineStore& a_store, IHearingCheck& a_hearing, RepeatGuard& a_repeats) :
		_settings(a_settings),
		_store(a_store),
		_hearing(a_hearing),
		_repeats(a_repeats)
	{
	}

	void TopicLineCatcher::Start()
	{
		if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
			holder->AddEventSink<RE::TESTopicInfoEvent>(this);
			logs::info("Listening for spoken lines");
		}
	}

	RE::BSEventNotifyControl TopicLineCatcher::ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>*)
	{
		if (!a_event || a_event->type != RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin || !_settings.IsCatching()) {
			return RE::BSEventNotifyControl::kContinue;
		}
		auto* speaker = a_event->speakerRef.get();
		auto* info = RE::TESForm::LookupByID<RE::TESTopicInfo>(a_event->topicInfoFormID);
		if (!speaker || !info) {
			return RE::BSEventNotifyControl::kContinue;
		}
		const auto facts = GameLineReader::ReadFacts(speaker, info);
		if (!IsWorthKeeping(facts) || !_repeats.IsNew(speaker->GetFormID(), info->GetFormID())) {
			return RE::BSEventNotifyControl::kContinue;
		}
		auto line = GameLineReader::ReadLine(speaker, info, facts);
		if (!line) {
			return RE::BSEventNotifyControl::kContinue;
		}
		logs::debug("Caught {}{}: \"{}\" (info {:08X}, scene {:08X})", line->speaker.name, line->menu ? " (menu talk)" : "", line->words,
			info->GetFormID(), line->scene);
		// Background chatter check (temporary): at info level so it shows in every build
		if (!line->menu) {
			logs::info("Chatter check: {}: \"{}\" | {}", line->speaker.name, line->words, GameLineReader::DescribeSource(speaker, info));
		}

		const auto handle = speaker->GetHandle();
		SKSE::GetTaskInterface()->AddTask([this, handle, heard = std::move(*line)]() mutable {
			const auto settings = _settings.Get();
			const auto ref = handle.get();
			if (!settings || !ref) {
				return;
			}
			GameActorScan scan;
			const auto    hearing = _hearing.Check(heard.speaker, GameLineReader::PointOf(ref->GetPosition()), heard.to, scan, *settings);
			heard.heardBy = hearing.heardBy;
			heard.playerThere = hearing.playerThere;
			_store.Add(std::move(heard), static_cast<std::size_t>(settings->mostWaiting));
		});
		return RE::BSEventNotifyControl::kContinue;
	}
}
