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

	[[nodiscard]] bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept;
	[[nodiscard]] bool CaptureSystemMemory(SystemMemorySnapshot& a_snapshot) noexcept;

	void StartTelemetryWorker();
	void StopTelemetryWorker() noexcept;
	[[nodiscard]] bool GetLatestTelemetry(
		ProcessMemorySnapshot& a_process,
		SystemMemorySnapshot& a_system) noexcept;
}
