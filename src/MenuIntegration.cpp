#include "MenuIntegration.h"
#include "FrameMonitor.h"
#include "PCH.h"
#include "ProcessMemory.h"

#include <format>

namespace menu
{
	namespace
	{
		using AddSectionItemFunction = void (*)(const char*, void(__stdcall*)());
		using GetVersionFunction = float (*)();
		using TextFunction = void (*)(const char*, const char*);

		HMODULE       g_framework = nullptr;
		TextFunction  g_text = nullptr;

		void __stdcall RenderMonitorPage()
		{
			if (!g_framework || !g_text) {
				return;
			}
			const auto text = g_text;

			const auto frame = monitor::GetFrameSnapshot();
			if (frame.available) {
				const auto fpsLine = std::format("Main-loop cadence: {:.1f} frames/s", frame.framesPerSecond);
				const auto frameLine = std::format("Main-loop interval: {:.2f} ms", frame.frameTimeMilliseconds);
				const auto countLine = std::format("Samples: {}", frame.frameCount);
				text(fpsLine.c_str(), nullptr);
				text(frameLine.c_str(), nullptr);
				text(countLine.c_str(), nullptr);
			} else {
				text("Frame-time monitor disabled until its engine hook is verified.", nullptr);
			}
			text("This is Skyrim main-loop cadence, not GPU-present/display FPS.", nullptr);

			memory::ProcessMemorySnapshot process{};
			memory::SystemMemorySnapshot  system{};
			if (memory::GetLatestTelemetry(process, system)) {
				constexpr double mib = 1024.0 * 1024.0;
				const auto       processLine = std::format(
					"Process RAM: working set {:.0f} MiB | private {:.0f} MiB",
					static_cast<double>(process.workingSetBytes) / mib,
					static_cast<double>(process.privateUsageBytes) / mib);
				const auto systemLine = std::format(
					"System RAM: {}% used | {:.0f} MiB available",
					system.memoryLoadPercent,
					static_cast<double>(system.availablePhysicalBytes) / mib);
				text(processLine.c_str(), nullptr);
				text(systemLine.c_str(), nullptr);
			} else {
				text("Memory telemetry: waiting for samples", nullptr);
			}
			text("Visual quality and game settings are not modified.", nullptr);
		}
	}

	bool RegisterMenuPage() noexcept
	{
		g_framework = ::GetModuleHandleW(L"SKSEMenuFramework.dll");
		if (!g_framework) {
			logger::info("SKSE Menu Framework not loaded; optional monitor page skipped");
			return false;
		}

		const auto addSectionItem = reinterpret_cast<AddSectionItemFunction>(
			::GetProcAddress(g_framework, "AddSectionItem"));
		const auto getVersion = reinterpret_cast<GetVersionFunction>(
			::GetProcAddress(g_framework, "GetMenuFrameworkVersion"));
		g_text = reinterpret_cast<TextFunction>(::GetProcAddress(g_framework, "igTextUnformatted"));
		if (!addSectionItem || !getVersion || !g_text) {
			logger::warn("SKSE Menu Framework is missing required API exports; monitor page skipped");
			return false;
		}

		const auto version = getVersion();
		if (version < 3.0F) {
			logger::warn("SKSE Menu Framework version {:.2f} is older than the supported API; monitor page skipped", version);
			return false;
		}

		addSectionItem("SkyrimFPSMemoryCore/Monitor", RenderMonitorPage);
		logger::info("Registered SkyrimFPSMemoryCore monitor page with SKSE Menu Framework {:.2f}", version);
		return true;
	}
}
