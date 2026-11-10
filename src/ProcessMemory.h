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

	struct SystemMemorySnapshot
	{
		std::uint64_t totalPhysicalBytes{ 0 };
		std::uint64_t availablePhysicalBytes{ 0 };
		std::uint64_t totalPageFileBytes{ 0 };
		std::uint64_t availablePageFileBytes{ 0 };
		std::uint32_t memoryLoadPercent{ 0 };
	};

	// Read-only process telemetry. This deliberately does not trim or modify
	// the game's working set.
	[[nodiscard]] bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept;

	// Read-only system-wide memory pressure counters from GlobalMemoryStatusEx.
	[[nodiscard]] bool CaptureSystemMemory(SystemMemorySnapshot& a_snapshot) noexcept;
}
