#pragma once

#include <cstdint>

namespace monitor
{
	struct FrameSnapshot
	{
		double        frameTimeMilliseconds{ 0.0 };
		double        framesPerSecond{ 0.0 };
		std::uint64_t frameCount{ 0 };
		bool          available{ false };
	};

	[[nodiscard]] bool          InstallMainLoopHook();
	[[nodiscard]] FrameSnapshot GetFrameSnapshot() noexcept;
}
