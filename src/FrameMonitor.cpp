#include "FrameMonitor.h"
#include "PCH.h"

#include <dxgi.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

namespace monitor
{
	namespace
	{
		using PresentFunction = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);

		constexpr std::size_t kPresentVtableIndex = 8;

		PresentFunction              g_originalPresent = nullptr;
		std::atomic<IDXGISwapChain*> g_hookedSwapChain{ nullptr };
		std::atomic<bool>            g_installed{ false };
		std::atomic<std::int64_t>    g_lastPresentNanoseconds{ 0 };
		std::atomic<std::int64_t>    g_frameIntervalNanoseconds{ 0 };
		std::atomic<std::uint64_t>   g_frameCount{ 0 };

		static_assert(std::atomic_ref<void*>::is_always_lock_free);

		bool IsReadableProtection(DWORD a_protection) noexcept
		{
			switch (a_protection & 0xFF) {
			case PAGE_READONLY:
			case PAGE_READWRITE:
			case PAGE_WRITECOPY:
			case PAGE_EXECUTE_READ:
			case PAGE_EXECUTE_READWRITE:
			case PAGE_EXECUTE_WRITECOPY:
				return true;
			default:
				return false;
			}
		}

		HRESULT STDMETHODCALLTYPE PresentHook(
			IDXGISwapChain* a_swapChain,
			UINT            a_syncInterval,
			UINT            a_flags) noexcept
		{
			if (a_swapChain == g_hookedSwapChain.load(std::memory_order_relaxed) &&
				(a_flags & DXGI_PRESENT_TEST) == 0) {
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

		MEMORY_BASIC_INFORMATION objectInfo{};
		if (::VirtualQuery(swapChain, &objectInfo, sizeof(objectInfo)) != sizeof(objectInfo) ||
			objectInfo.State != MEM_COMMIT ||
			!IsReadableProtection(objectInfo.Protect)) {
			logger::error("DXGI Present monitor skipped: swap-chain object memory is not readable");
			return false;
		}

		auto** vtable = *reinterpret_cast<void***>(swapChain);
		if (!vtable) {
			logger::error("DXGI Present monitor skipped: swap-chain vtable is null");
			return false;
		}

		const auto     vtableAddress = reinterpret_cast<std::uintptr_t>(vtable);
		constexpr auto slotOffset = kPresentVtableIndex * sizeof(void*);
		if (vtableAddress > std::numeric_limits<std::uintptr_t>::max() - slotOffset - sizeof(void*)) {
			logger::error("DXGI Present monitor skipped: swap-chain vtable address overflow");
			return false;
		}
		auto** presentSlot = reinterpret_cast<void**>(vtableAddress + slotOffset);
		if (reinterpret_cast<std::uintptr_t>(presentSlot) % std::atomic_ref<void*>::required_alignment != 0) {
			logger::error("DXGI Present monitor skipped: Present vtable slot is not correctly aligned");
			return false;
		}

		MEMORY_BASIC_INFORMATION vtableInfo{};
		if (::VirtualQuery(presentSlot, &vtableInfo, sizeof(vtableInfo)) != sizeof(vtableInfo) ||
			vtableInfo.State != MEM_COMMIT ||
			!IsReadableProtection(vtableInfo.Protect)) {
			logger::error("DXGI Present monitor skipped: Present vtable slot is not readable");
			return false;
		}

		auto& presentEntry = *presentSlot;
		if (!presentEntry || presentEntry == reinterpret_cast<void*>(&PresentHook)) {
			logger::error("DXGI Present monitor skipped: Present vtable entry is invalid or already hooked");
			return false;
		}

		DWORD oldProtection{};
		if (!::VirtualProtect(presentSlot, sizeof(void*), PAGE_READWRITE, &oldProtection)) {
			logger::error("DXGI Present monitor skipped: could not make Present vtable slot writable");
			return false;
		}

		g_originalPresent = reinterpret_cast<PresentFunction>(presentEntry);
		g_hookedSwapChain.store(swapChain, std::memory_order_relaxed);

		std::atomic_ref<void*> presentSlotReference(presentEntry);
		presentSlotReference.store(reinterpret_cast<void*>(&PresentHook), std::memory_order_release);

		DWORD ignoredProtection{};
		if (!::VirtualProtect(presentSlot, sizeof(void*), oldProtection, &ignoredProtection)) {
			logger::error("DXGI Present monitor installed, but original vtable page protection could not be restored");
		}

		g_installed.store(true, std::memory_order_release);
		logger::info(
			"DXGI Present monitor installed on the primary swap chain; measuring Present-call cadence and chaining the existing Present function");
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
