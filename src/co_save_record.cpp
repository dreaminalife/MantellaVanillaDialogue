#include "co_save_record.h"

namespace HeardLines
{
	CoSaveRecord::CoSaveRecord(SKSE::SerializationInterface* a_intfc) :
		_intfc(a_intfc)
	{
	}

	void CoSaveRecord::Write(std::uint32_t a_version, const std::string& a_data)
	{
		if (!_intfc->OpenRecord(kType, a_version) || !_intfc->WriteRecordData(a_data.data(), static_cast<std::uint32_t>(a_data.size()))) {
			logs::error("Could not save the waiting lines");
		}
	}

	std::optional<std::pair<std::uint32_t, std::string>> CoSaveRecord::Read()
	{
		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kType) {
				continue;
			}
			std::string data(length, '\0');
			const auto  read = _intfc->ReadRecordData(data.data(), length);
			data.resize(read);  // A cut-off record reads short, and the store then skips it
			return std::make_pair(version, std::move(data));
		}
		return std::nullopt;
	}
}
