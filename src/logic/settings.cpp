#include "logic/settings.h"

namespace HeardLines
{
	bool SettingsHolder::Set(bool a_catchLines, bool a_nearbyOverhear, float a_hearingDistance, int a_mostWaiting)
	{
		if (!(a_hearingDistance > 0) || a_mostWaiting <= 0) {
			return false;
		}
		std::scoped_lock lock(_lock);
		_settings = Settings{ a_catchLines, a_nearbyOverhear, a_hearingDistance, a_mostWaiting };
		return true;
	}

	std::optional<Settings> SettingsHolder::Get() const
	{
		std::scoped_lock lock(_lock);
		return _settings;
	}

	bool SettingsHolder::IsCatching() const
	{
		std::scoped_lock lock(_lock);
		return _settings && _settings->catchLines;
	}
}
