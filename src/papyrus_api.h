#pragma once

#include "logic/line_filter.h"
#include "logic/line_store.h"
#include "logic/settings.h"

namespace HeardLines::PapyrusApi
{
	// Scriptname MantellaHeardLines Hidden
	// Bool Function SetSettings(Bool catchLines, Bool nearbyOverhear, Float hearingDistance, Int mostWaiting) Global Native
	// String Function TakeLines() Global Native
	void Register(SettingsHolder& a_settings, ILineStore& a_store);
}
