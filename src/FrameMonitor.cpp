#include "FrameMonitor.h"
#include "PCH.h"

namespace monitor
{
	bool InstallMainLoopHook() noexcept
	{
		const auto runtime = REL::Module::get().version();
		logger::warn(
			"Frame monitoring is disabled for runtime {}.{}.{}.{}: no verified render/present hook is available yet",
			runtime[0], runtime[1], runtime[2], runtime[3]);
		return false;
	}

	FrameSnapshot GetFrameSnapshot() noexcept
	{
		return {};
	}
}
