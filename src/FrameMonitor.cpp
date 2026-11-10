#include "FrameMonitor.h"
#include "PCH.h"

#include <dxgi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace monitor
{
	namespace
	{
		using PresentFunction = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

		constexpr std::size_t kPresentVtableIndex = 8;
		constexpr std::size_t kSwapChainVtableSize = 18;

		std::array<void*, kSwapChainVtableSize> g_shadowVtable{};
		PresentFunction                        g_originalPresent = nullptr;
		std::atomic<bool>                      g_installed{ false };
		std::atomic<std::int64_t>               g_lastPresentNanoseconds{ 0 };
		std::atomic<std::int64_t>               g_frameIntervalNanoseconds{ 0 };
		std::atomic<std::uint64_t>              g_frameCount{ 0 };

		static_assert(std::atomic_ref<void*>::is_always_lock_free);

		HRESULT STDMETHODCALLTYPE PresentHook(
			IDXGISwapChain* a_swapChain,
			UINT            a_syncInterval,
			UINT            a_flags) noexcept
		{
			if ((a_flags & DXGI_PRESENT_TEST) == 0) {
				const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::chrono::steady_clock::now().time_since_epoch()).count();
				const auto previous = g_lastPresentNanoseconds.exchange(now, std::memory_order_relaxed);

				if (previous > 0 && now > previous) {
					const auto interval = now - previous;
					if (interval < 10'000'000'000LL) {
						g_frameIntervalNanoseconds.store(interval, std::memory_order_relaxed);
						g_frameCount.fetch_add(1, std::memory_order_relaxed);
					}
				}
			}

			const auto original = g_originalPresent;
			return original ? original(a_swapChain, a_syncInterval, a_flags) : E_FAIL;
		}
	}

	bool InstallPresentHook()
	{
		if (g_installed.load(std::memory_order_acquire)) {
			return true;
		}

		const auto runtime = REL::Module::get().version();
		const bool supportedSE = REL::Module::IsSE() && runtime == REL::Version{ 1, 5, 97, 0 };
		const bool supportedAE = REL::Module::IsAE();
		if (!supportedSE && !supportedAE) {
			logger::warn(
				"DXGI Present monitor skipped on unsupported runtime {}.{}.{}.{}",
				runtime[0], runtime[1], runtime[2], runtime[3]);
			return false;
		}

		auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) {
			logger::warn("DXGI Present monitor skipped: BSGraphics::Renderer is not available yet");
			return false;
		}

		auto& renderData = renderer->GetRuntimeData();
		auto* swapChain = reinterpret_cast<IDXGISwapChain*>(renderData.renderWindows[0].swapChain);
		if (!swapChain) {
			logger::warn("DXGI Present monitor skipped: the primary swap chain is not available");
			return false;
		}

		auto** originalVtable = *reinterpret_cast<void***>(swapChain);
		if (!originalVtable) {
			logger::error("DXGI Present monitor skipped: swap-chain vtable is null");
			return false;
		}
		auto& objectVptr = *reinterpret_cast<void**>(swapChain);

		MEMORY_BASIC_INFORMATION memoryInfo{};
		if (::VirtualQuery(&objectVptr, &memoryInfo, sizeof(memoryInfo)) != sizeof(memoryInfo) ||
			memoryInfo.State != MEM_COMMIT) {
			logger::error("DXGI Present monitor skipped: could not validate swap-chain object memory");
			return false;
		}

		const auto protection = memoryInfo.Protect & 0xFF;
		const bool writable = protection == PAGE_READWRITE ||
		                      protection == PAGE_WRITECOPY ||
		                      protection == PAGE_EXECUTE_READWRITE ||
		                      protection == PAGE_EXECUTE_WRITECOPY;
		if (!writable) {
			logger::error("DXGI Present monitor skipped: swap-chain vptr memory is not writable");
			return false;
		}

		if (!originalVtable[kPresentVtableIndex]) {
			logger::error("DXGI Present monitor skipped: original Present function is null");
			return false;
		}

		std::copy_n(originalVtable, g_shadowVtable.size(), g_shadowVtable.begin());
		g_originalPresent = reinterpret_cast<PresentFunction>(originalVtable[kPresentVtableIndex]);
		g_shadowVtable[kPresentVtableIndex] = reinterpret_cast<void*>(&PresentHook);

		if (reinterpret_cast<std::uintptr_t>(&objectVptr) % std::atomic_ref<void*>::required_alignment != 0) {
			g_originalPresent = nullptr;
			logger::error("DXGI Present monitor skipped: swap-chain vptr is not correctly aligned");
			return false;
		}

		std::atomic_ref<void*> vptrReference(objectVptr);
		vptrReference.store(g_shadowVtable.data(), std::memory_order_release);
		g_installed.store(true, std::memory_order_release);

		logger::info(
			"DXGI Present monitor installed on the primary swap chain; measuring Present-call cadence without patching game code");
		return true;
	}

	FrameSnapshot GetFrameSnapshot() noexcept
	{
		const auto interval = g_frameIntervalNanoseconds.load(std::memory_order_relaxed);
		const auto count = g_frameCount.load(std::memory_order_relaxed);
		if (interval <= 0 || count == 0) {
			return {};
		}

		const auto intervalNanoseconds = static_cast<double>(interval);
		return {
			intervalNanoseconds / 1'000'000.0,
			1'000'000'000.0 / intervalNanoseconds,
			count,
			true
		};
	}
}
