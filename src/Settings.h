#pragma once

namespace config
{
	struct Settings
	{
		bool enableFrameMonitor{ true };
		bool enableMemoryTelemetry{ true };
		bool enableMenuFrameworkPage{ true };
	};

	[[nodiscard]] Settings LoadSettings();
	[[nodiscard]] const Settings& GetSettings() noexcept;
}
