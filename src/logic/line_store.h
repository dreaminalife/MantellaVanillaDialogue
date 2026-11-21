#pragma once

#include "logic/heard_line.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace HeardLines
{
	// One record in the save (the SKSE co-save in game, memory in tests)
	class ISaveRecord
	{
	public:
		virtual ~ISaveRecord() = default;
		virtual void Write(std::uint32_t a_version, const std::string& a_data) = 0;
		// The record's version and data, or nothing when the save has none
		virtual std::optional<std::pair<std::uint32_t, std::string>> Read() = 0;
	};

	// The lines caught since the game add-on last asked for them
	class ILineStore
	{
	public:
		virtual ~ILineStore() = default;
		// Past a_mostWaiting lines, the oldest are dropped
		virtual void Add(HeardLine a_line, std::size_t a_mostWaiting) = 0;
		// Every waiting line as the JSON Mantella reads, and the store is emptied. a_now is left out when the game can't tell the time
		virtual std::string TakeAllAsJson(const std::optional<GameTime>& a_now, const std::string& a_player) = 0;
		virtual void        Save(ISaveRecord& a_record) = 0;
		// A record of an unknown version, or a cut-off one, is skipped
		virtual bool Load(ISaveRecord& a_record) = 0;
		// A new game, or before a save loads
		virtual void        Clear() = 0;
		[[nodiscard]] virtual std::size_t Count() = 0;
	};

	class CoSaveLineStore : public ILineStore
	{
	public:
		static constexpr std::uint32_t kVersion = 1;

		void        Add(HeardLine a_line, std::size_t a_mostWaiting) override;
		std::string TakeAllAsJson(const std::optional<GameTime>& a_now, const std::string& a_player) override;
		void        Save(ISaveRecord& a_record) override;
		bool        Load(ISaveRecord& a_record) override;
		void        Clear() override;
		std::size_t Count() override;

	private:
		std::mutex            _lock;
		std::deque<HeardLine> _lines;
	};
}
