#include "PCH.h"
#include "ProcessMemory.h"

#include <psapi.h>

namespace memory
{
	bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept
	{
		PROCESS_MEMORY_COUNTERS_EX counters{};
		counters.cb = sizeof(counters);

		if (!::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
			return false;
		}

		a_snapshot.workingSetBytes = counters.WorkingSetSize;
		a_snapshot.peakWorkingSetBytes = counters.PeakWorkingSetSize;
		a_snapshot.privateUsageBytes = counters.PrivateUsage;
		a_snapshot.pagefileUsageBytes = counters.PagefileUsage;
		return true;
	}

	bool CaptureSystemMemory(SystemMemorySnapshot& a_snapshot) noexcept
	{
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);

		if (!::GlobalMemoryStatusEx(&status)) {
			return false;
		}

		a_snapshot.totalPhysicalBytes = status.ullTotalPhys;
		a_snapshot.availablePhysicalBytes = status.ullAvailPhys;
		a_snapshot.totalPageFileBytes = status.ullTotalPageFile;
		a_snapshot.availablePageFileBytes = status.ullAvailPageFile;
		a_snapshot.memoryLoadPercent = status.dwMemoryLoad;
		return true;
	}
}
