#pragma once

#include <mutex>
#include <optional>

namespace HeardLines
{
	struct Settings
	{
		bool  catchLines = false;      // Vanilla Dialogues on
		bool  nearbyOverhear = true;   // NPCs near a line also hear it, not only the speaker and the person spoken to
		float hearingDistance = 0;     // Game units
		int   mostWaiting = 0;         // Lines kept until handed over. Past it, chatter is dropped first, then the oldest
	};

	// The settings the game add-on sends on every load. Nothing is caught until they arrive
	class SettingsHolder
	{
	public:
		// False, and nothing changes, when the distance or the most waiting lines aren't above 0
		bool Set(bool a_catchLines, bool a_nearbyOverhear, float a_hearingDistance, int a_mostWaiting);

		// Empty until settings arrived
		[[nodiscard]] std::optional<Settings> Get() const;

		// Settings arrived and Vanilla Dialogues is on
		[[nodiscard]] bool IsCatching() const;

	private:
		mutable std::mutex      _lock;
		std::optional<Settings> _settings;
	};
}
