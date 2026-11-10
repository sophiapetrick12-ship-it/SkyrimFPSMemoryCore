#include "FrameMonitor.h"
#include "MenuIntegration.h"
#include "PCH.h"
#include "ProcessMemory.h"
#include "Settings.h"

#include <exception>

namespace
{
	void SetupLog()
	{
		auto logsFolder = logger::log_directory();
		if (!logsFolder) {
			util::report_and_fail("SKSE log_directory not provided, logs can't be written");
		}

		const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
		const auto  logName = plugin ? std::string{ plugin->GetName() } + ".log" : "Plugin.log";
		auto        logPath = *logsFolder / logName;

		auto                          fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
		std::vector<spdlog::sink_ptr> sinks{ fileSink };
		if (IsDebuggerPresent()) {
			sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
		}

		auto spdlogger = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
		spdlog::set_default_logger(std::move(spdlogger));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] [%s:%#] %v");
#ifdef NDEBUG
		spdlog::set_level(spdlog::level::info);
#else
		spdlog::set_level(spdlog::level::trace);
#endif
		spdlog::flush_on(spdlog::level::info);
	}

	void OnDataLoaded()
	{
		logger::info("OnDataLoaded: initializing optional modules");

		config::Settings settings{};
		try {
			settings = config::LoadSettings();
		} catch (const std::exception& error) {
			logger::error("Failed to load configuration; continuing with safe defaults: {}", error.what());
		} catch (...) {
			logger::error("Failed to load configuration; continuing with safe defaults");
		}

		if (settings.enableMemoryTelemetry) {
			try {
				memory::StartTelemetryWorker();
			} catch (const std::exception& error) {
				logger::error("Memory telemetry could not start: {}", error.what());
			} catch (...) {
				logger::error("Memory telemetry could not start");
			}
		}

		if (settings.enableFrameMonitor) {
			try {
				if (!monitor::InstallPresentHook()) {
					logger::warn("Frame monitoring remains unavailable; the DXGI Present hook could not be installed");
				}
			} catch (const std::exception& error) {
				logger::error("Frame monitor initialization failed safely: {}", error.what());
			} catch (...) {
				logger::error("Frame monitor initialization failed safely");
			}
		}

		if (settings.enableMenuFrameworkPage) {
			try {
				menu::RegisterMenuPage();
			} catch (const std::exception& error) {
				logger::error("Menu page registration failed: {}", error.what());
			} catch (...) {
				logger::error("Menu page registration failed");
			}
		}
	}

	void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			OnDataLoaded();
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	if (!plugin) {
		logger::error("Failed to get plugin declaration");
		return false;
	}
	logger::info("{} v{} loaded", plugin->GetName(), plugin->GetVersion());

	const auto runtime = REL::Module::get().version();
	logger::info("Detected Skyrim runtime {}.{}.{}.{}", runtime[0], runtime[1], runtime[2], runtime[3]);

	const auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging) {
		logger::error("Failed to get SKSE messaging interface");
		return false;
	}

	if (!messaging->RegisterListener(OnSKSEMessage)) {
		logger::error("Failed to register messaging listener");
		return false;
	}

	return true;
}
