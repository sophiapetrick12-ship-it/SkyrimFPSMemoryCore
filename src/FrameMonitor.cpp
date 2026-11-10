#include "FrameMonitor.h"
#include "PCH.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>

namespace monitor
{
	namespace
	{
		using UpdateFunction = void(RE::Main*);
		REL::Relocation<UpdateFunction*> g_originalUpdate;

		std::atomic<double>        g_frameTimeMilliseconds{ 0.0 };
		std::atomic<double>        g_framesPerSecond{ 0.0 };
		std::atomic<std::uint64_t> g_frameCount{ 0 };
		std::atomic<bool>          g_available{ false };
		std::atomic<bool>          g_installed{ false };

		void MainUpdateHook(RE::Main* a_main)
		{
			using clock = std::chrono::steady_clock;
			static auto previousFrameStart = clock::time_point{};

			const auto currentFrameStart = clock::now();
			if (previousFrameStart.time_since_epoch().count() != 0) {
				const auto elapsed = std::chrono::duration<double, std::milli>(currentFrameStart - previousFrameStart).count();
				if (elapsed > 0.0 && elapsed < 10000.0) {
					g_frameTimeMilliseconds.store(elapsed, std::memory_order_relaxed);
					g_framesPerSecond.store(1000.0 / elapsed, std::memory_order_relaxed);
					g_frameCount.fetch_add(1, std::memory_order_relaxed);
					g_available.store(true, std::memory_order_release);
				}
			}
			previousFrameStart = currentFrameStart;

			g_originalUpdate(a_main);
		}
	}

	bool InstallMainLoopHook() noexcept
	{
		if (g_installed.load(std::memory_order_acquire)) {
			return true;
		}

		const auto runtime = REL::Module::get().version();
		const bool   supportedSE = REL::Module::IsSE() && runtime == REL::Version{ 1, 5, 97, 0 };
		const bool   supportedAE = REL::Module::IsAE();
		if (!supportedSE && !supportedAE) {
			logger::warn(
				"Main-loop timing disabled for unsupported runtime {}.{}.{}.{}",
				runtime[0], runtime[1], runtime[2], runtime[3]);
			return false;
		}

		// Resolve the call site through Address Library for this exact runtime.
		// Missing IDs/offsets fail closed before any executable memory is changed.
		const REL::VariantID     callSiteID{ 35551, 36544, 0 };
		const REL::VariantOffset callSiteOffset{ 0x11F, 0x160, 0 };
		if (callSiteID.offset() == 0 || callSiteOffset.offset() == 0) {
			logger::warn(
				"No validated main-loop relocation for runtime {}.{}.{}.{}; timing disabled",
				runtime[0], runtime[1], runtime[2], runtime[3]);
			return false;
		}
		REL::Relocation<std::uintptr_t> callSite{ callSiteID, callSiteOffset };

		const auto site = callSite.address();
		const auto text = REL::Module::get().segment(REL::Segment::textx);
		const auto textBegin = text.address();
		const auto textEnd = textBegin + text.size();
		if (!site || site < textBegin || site > textEnd || textEnd - site < 5) {
			logger::error("Main-loop hook call site is outside executable code; hook disabled");
			return false;
		}

		std::array<std::uint8_t, 5> instruction{};
		std::memcpy(instruction.data(), reinterpret_cast<const void*>(site), instruction.size());
		if (instruction[0] != 0xE8) {
			logger::error("Main-loop hook signature mismatch (expected CALL rel32); hook disabled");
			return false;
		}

		std::int32_t relativeTarget = 0;
		std::memcpy(&relativeTarget, instruction.data() + 1, sizeof(relativeTarget));
		const auto destination = static_cast<std::uintptr_t>(
			static_cast<std::intptr_t>(site + instruction.size()) + relativeTarget);
		if (destination < textBegin || destination >= textEnd) {
			logger::error("Main-loop CALL destination is outside executable code; hook disabled");
			return false;
		}

		SKSE::AllocTrampoline(14);
		g_originalUpdate = SKSE::GetTrampoline().write_call<5>(site, MainUpdateHook);
		g_installed.store(true, std::memory_order_release);
		logger::info(
			"Main-loop timing hook installed for runtime {}.{}.{}.{} after call-site validation",
			runtime[0], runtime[1], runtime[2], runtime[3]);
		return true;
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
