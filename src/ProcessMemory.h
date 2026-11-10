#pragma once

#include <cstdint>

namespace memory
{
	struct ProcessMemorySnapshot
	{
		std::uint64_t workingSetBytes{ 0 };
		std::uint64_t peakWorkingSetBytes{ 0 };
		std::uint64_t privateUsageBytes{ 0 };
		std::uint64_t pagefileUsageBytes{ 0 };
	};

	// Read-only process telemetry. This deliberately does not trim or modify
	// the game's working set.
	[[nodiscard]] bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept;
}
