#pragma once

#include "logic/hearing.h"
#include "logic/line_filter.h"
#include "logic/line_store.h"
#include "logic/settings.h"

namespace HeardLines
{
	// Catches lines as NPCs start them
	class ILineCatcher
	{
	public:
		virtual ~ILineCatcher() = default;
		virtual void Start() = 0;
	};

	// Listens to Skyrim's report of each line that starts (TESTopicInfoEvent). Nothing runs while nobody speaks.
	// The line is read where the game reports it. Who heard it is worked out on the main thread, where actors can be read safely
	class TopicLineCatcher :
		public ILineCatcher,
		public RE::BSTEventSink<RE::TESTopicInfoEvent>
	{
	public:
		TopicLineCatcher(SettingsHolder& a_settings, ILineStore& a_store, IHearingCheck& a_hearing, RepeatGuard& a_repeats);

		void Start() override;

		RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override;

	private:
		SettingsHolder& _settings;
		ILineStore&     _store;
		IHearingCheck&  _hearing;
		RepeatGuard&    _repeats;
	};
}
