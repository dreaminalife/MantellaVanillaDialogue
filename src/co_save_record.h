#pragma once

#include "logic/line_store.h"

namespace HeardLines
{
	// The waiting lines' record in the SKSE co-save, so closing the game before the next request loses nothing
	class CoSaveRecord : public ISaveRecord
	{
	public:
		static constexpr std::uint32_t kUniqueID = 'MHLN';
		static constexpr std::uint32_t kType = 'LINS';

		explicit CoSaveRecord(SKSE::SerializationInterface* a_intfc);

		void                                                Write(std::uint32_t a_version, const std::string& a_data) override;
		std::optional<std::pair<std::uint32_t, std::string>> Read() override;

	private:
		SKSE::SerializationInterface* _intfc;
	};
}
