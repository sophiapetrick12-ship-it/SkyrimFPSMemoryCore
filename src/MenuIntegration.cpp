#include "MenuIntegration.h"
#include "FrameMonitor.h"
#include "PCH.h"
#include "ProcessMemory.h"

#include <atomic>
#include <format>

namespace menu
{
	namespace
	{
		using AddSectionItemFunction = void (*)(const char*, void(__stdcall*)());
		using GetVersionFunction = float (*)();
		using TextFunction = void (*)(const char*, const char*);

		struct MenuState
		{
			std::atomic<TextFunction> text{ nullptr };
		};

		MenuState& GetState() noexcept
		{
			static MenuState state;
			return state;
		}

		void __stdcall RenderMonitorPage()
		{
			const auto text = GetState().text.load(std::memory_order_acquire);
			if (!text) {
				return;
			}

			const auto frame = monitor::GetFrameSnapshot();
			if (frame.available) {
				const auto fpsLine = std::format("DXGI Present rate: {:.1f} FPS", frame.framesPerSecond);
				const auto frameLine = std::format("DXGI Present interval: {:.2f} ms", frame.frameTimeMilliseconds);
				const auto countLine = std::format("Samples: {}", frame.frameCount);
				text(fpsLine.c_str(), nullptr);
				text(frameLine.c_str(), nullptr);
				text(countLine.c_str(), nullptr);
			} else {
				text("No Present samples available (monitor disabled or swap chain unavailable).", nullptr);
			}
			text("Measures Skyrim swap-chain Present cadence, not physical display scan-out.", nullptr);

			memory::ProcessMemorySnapshot process{};
			memory::SystemMemorySnapshot  system{};
			if (memory::GetLatestTelemetry(process, system)) {
				constexpr double mib = 1024.0 * 1024.0;
				const auto       processLine = std::format(
					"Process: working set {:.0f} MiB | private {:.0f} MiB | commit {:.0f} MiB",
					static_cast<double>(process.workingSetBytes) / mib,
					static_cast<double>(process.privateUsageBytes) / mib,
					static_cast<double>(process.commitChargeBytes) / mib);
				const auto systemLine = std::format(
					"System RAM: {}% used | {:.0f} MiB available",
					system.memoryLoadPercent,
					static_cast<double>(system.availablePhysicalBytes) / mib);
				const auto commitLine = std::format(
					"System commit: {:.0f} MiB available / {:.0f} MiB limit",
					static_cast<double>(system.availableCommitBytes) / mib,
					static_cast<double>(system.commitLimitBytes) / mib);
				text(processLine.c_str(), nullptr);
				text(systemLine.c_str(), nullptr);
				text(commitLine.c_str(), nullptr);
			} else {
				text("Memory telemetry: waiting for samples", nullptr);
			}
			text("Visual quality and game settings are not modified.", nullptr);
		}
	}

	bool RegisterMenuPage()
	{
		const auto framework = ::GetModuleHandleW(L"SKSEMenuFramework");
		if (!framework) {
			logger::info("SKSE Menu Framework not loaded; optional monitor page skipped");
			return false;
		}

		const auto addSectionItem = reinterpret_cast<AddSectionItemFunction>(
			::GetProcAddress(framework, "AddSectionItem"));
		const auto getVersion = reinterpret_cast<GetVersionFunction>(
			::GetProcAddress(framework, "GetMenuFrameworkVersion"));
		const auto text = reinterpret_cast<TextFunction>(
			::GetProcAddress(framework, "igTextUnformatted"));
		if (!addSectionItem || !getVersion || !text) {
			logger::warn("SKSE Menu Framework is missing required API exports; monitor page skipped");
			return false;
		}

		const auto version = getVersion();
		if (version < 3.0F) {
			logger::warn("SKSE Menu Framework version {:.2f} is older than the supported API; monitor page skipped", version);
			return false;
		}

		GetState().text.store(text, std::memory_order_release);
		addSectionItem("SkyrimFPSMemoryCore/Monitor", RenderMonitorPage);
		logger::info("Registered SkyrimFPSMemoryCore monitor page with SKSE Menu Framework {:.2f}", version);
		return true;
	}
}
