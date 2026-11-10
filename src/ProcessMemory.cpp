#include "PCH.h"
#include "ProcessMemory.h"

#include <psapi.h>

namespace memory
{
	bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept
	{
		PROCESS_MEMORY_COUNTERS_EX counters{};
		counters.cb = sizeof(counters);

		if (!::GetProcessMemoryInfo(
				::GetCurrentProcess(),
				reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
				sizeof(counters))) {
			return false;
		}

		a_snapshot.workingSetBytes = counters.WorkingSetSize;
		a_snapshot.peakWorkingSetBytes = counters.PeakWorkingSetSize;
		a_snapshot.privateUsageBytes = counters.PrivateUsage;
		a_snapshot.pagefileUsageBytes = counters.PagefileUsage;
		return true;
	}
}
