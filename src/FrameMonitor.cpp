#include "FrameMonitor.h"
#include "PCH.h"

#include <atomic>

namespace monitor
{
	namespace
	{
		std::atomic<double>        g_frameTimeMilliseconds{ 0.0 };
		std::atomic<double>        g_framesPerSecond{ 0.0 };
		std::atomic<std::uint64_t> g_frameCount{ 0 };
		std::atomic<bool>          g_available{ false };
	}

	bool InstallMainLoopHook() noexcept
	{
		const auto runtime = REL::Module::get().version();
		logger::warn(
			"Frame monitoring is disabled for runtime {}.{}.{}.{}: no verified engine hook is available yet",
			runtime[0], runtime[1], runtime[2], runtime[3]);
		return false;
	}

	FrameSnapshot GetFrameSnapshot() noexcept
	{
		return {
			g_frameTimeMilliseconds.load(std::memory_order_relaxed),
			g_framesPerSecond.load(std::memory_order_relaxed),
			g_frameCount.load(std::memory_order_relaxed),
			g_available.load(std::memory_order_acquire)
		};
	}
}
