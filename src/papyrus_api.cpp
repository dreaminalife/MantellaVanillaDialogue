#include "papyrus_api.h"

#include "game_line_reader.h"

namespace HeardLines::PapyrusApi
{
	namespace
	{
		SettingsHolder* g_settings = nullptr;
		ILineStore*     g_store = nullptr;

		// False, and nothing changes, when the distance or the most waiting lines aren't above 0
		bool SetSettings(RE::StaticFunctionTag*, bool a_catchLines, bool a_nearbyOverhear, float a_hearingDistance, std::int32_t a_mostWaiting)
		{
			const bool accepted = g_settings->Set(a_catchLines, a_nearbyOverhear, a_hearingDistance, a_mostWaiting);
			if (accepted) {
				logs::info("Settings: catch lines {}, nearby NPCs overhear {}, hearing distance {}, most waiting lines {}", a_catchLines, a_nearbyOverhear,
					a_hearingDistance, a_mostWaiting);
			} else {
				logs::warn("Settings refused: hearing distance {} and most waiting lines {} must be above 0", a_hearingDistance, a_mostWaiting);
			}
			return accepted;
		}

		// Every waiting line, and the game's time now, as JSON for Mantella. The lines are handed over once
		std::string TakeLines(RE::StaticFunctionTag*)
		{
			const auto now = GameLineReader::IsTimeKnown() ? std::optional<GameTime>(GameLineReader::Now()) : std::nullopt;
			return g_store->TakeAllAsJson(now, GameLineReader::PersonOf(RE::PlayerCharacter::GetSingleton()).name);
		}

		bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm)
		{
			constexpr auto script = "MantellaHeardLines"sv;
			// Callable from tasklets: they only touch the settings and the store, which have their own locks
			a_vm->RegisterFunction("SetSettings"sv, script, SetSettings, true);
			a_vm->RegisterFunction("TakeLines"sv, script, TakeLines, true);
			return true;
		}
	}

	void Register(SettingsHolder& a_settings, ILineStore& a_store)
	{
		g_settings = &a_settings;
		g_store = &a_store;
		SKSE::GetPapyrusInterface()->Register(RegisterFunctions);
	}
}
