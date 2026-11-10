#include "Settings.h"
#include "PCH.h"

#include <filesystem>

namespace config
{
	namespace
	{
		Settings g_settings{};

		std::filesystem::path GetSettingsPath()
		{
			const std::filesystem::path executablePath{ REL::Module::get().filePath() };
			return executablePath.parent_path() / "Data" / "SKSE" / "Plugins" / "SkyrimFPSMemoryCore.ini";
		}

		bool ReadBoolean(const std::filesystem::path& a_path, const wchar_t* a_key, bool a_default)
		{
			return ::GetPrivateProfileIntW(L"Features", a_key, a_default ? 1 : 0, a_path.c_str()) != 0;
		}
	}

	Settings LoadSettings()
	{
		g_settings = Settings{};
		const auto path = GetSettingsPath();

		g_settings.enableFrameMonitor = ReadBoolean(path, L"EnableFrameMonitor", true);
		g_settings.enableMemoryTelemetry = ReadBoolean(path, L"EnableMemoryTelemetry", true);
		g_settings.enableMenuFrameworkPage = ReadBoolean(path, L"EnableMenuFrameworkPage", true);

		std::error_code error;
		const bool exists = std::filesystem::exists(path, error);
		if (error) {
			logger::warn("Could not inspect the plugin configuration file; using defaults for missing values");
		} else if (!exists) {
			std::filesystem::create_directories(path.parent_path(), error);
			if (error) {
				logger::error("Could not create the plugin configuration directory: {}", error.message());
			} else {
				const bool wroteDefaults =
					::WritePrivateProfileStringW(L"Features", L"EnableFrameMonitor", L"1", path.c_str()) &&
					::WritePrivateProfileStringW(L"Features", L"EnableMemoryTelemetry", L"1", path.c_str()) &&
					::WritePrivateProfileStringW(L"Features", L"EnableMenuFrameworkPage", L"1", path.c_str());
				if (wroteDefaults) {
					logger::info("Created default plugin configuration");
				} else {
					logger::warn("Could not write the default plugin configuration file");
				}
			}
		}

		logger::info(
			"Settings: frame monitor={}, memory telemetry={}, menu page={}",
			g_settings.enableFrameMonitor,
			g_settings.enableMemoryTelemetry,
			g_settings.enableMenuFrameworkPage);
		return g_settings;
	}

	const Settings& GetSettings() noexcept
	{
		return g_settings;
	}
}
