// MantellaHeardLines: catches the voiced lines NPCs hear (scene lines and the player's talk in the dialogue menu) and keeps them
// in the save until the Mantella game add-on hands them to Mantella with a request it already sends. Mantella sorts them into the
// running conversation or each NPC's memory.
//
// The log is Documents\My Games\Skyrim Special Edition\SKSE\MantellaHeardLines.log

#include "co_save_record.h"
#include "line_catcher.h"
#include "papyrus_api.h"

namespace
{
	HeardLines::SettingsHolder       g_settings;
	HeardLines::CoSaveLineStore      g_store;
	HeardLines::DistanceHearingCheck g_hearing;
	HeardLines::RepeatGuard          g_repeats;
	HeardLines::TopicLineCatcher     g_catcher(g_settings, g_store, g_hearing, g_repeats);

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		HeardLines::CoSaveRecord record(a_intfc);
		g_store.Save(record);
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		HeardLines::CoSaveRecord record(a_intfc);
		g_repeats.Clear();
		if (g_store.Load(record)) {
			logs::info("Loaded {} waiting lines from the save", g_store.Count());
		}
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		g_store.Clear();
		g_repeats.Clear();
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	const auto* serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(HeardLines::CoSaveRecord::kUniqueID);
	serialization->SetSaveCallback(OnSave);
	serialization->SetLoadCallback(OnLoad);
	serialization->SetRevertCallback(OnRevert);

	HeardLines::PapyrusApi::Register(g_settings, g_store);
	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
		if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			g_catcher.Start();
		}
	});

	logs::info("MantellaHeardLines loaded");
	return true;
}
