#include "ProcessMemory.h"
#include "PCH.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <psapi.h>
#include <thread>

namespace memory
{
	namespace
	{
		class TelemetryWorker
		{
		public:
			~TelemetryWorker()
			{
				Stop();
			}

			void Start()
			{
				std::lock_guard lock(_mutex);
				if (_thread.joinable()) {
					return;
				}
				_stopping = false;
				_thread = std::thread([this] { Run(); });
			}

			void Stop() noexcept
			{
				{
					std::lock_guard lock(_mutex);
					_stopping = true;
				}
				_condition.notify_all();
				if (_thread.joinable()) {
					try {
						_thread.join();
					} catch (...) {
						// Do not allow shutdown-time telemetry cleanup to escape.
					}
				}
			}

			bool Read(ProcessMemorySnapshot& a_process, SystemMemorySnapshot& a_system) noexcept
			{
				std::lock_guard lock(_snapshotMutex);
				if (!_hasSnapshot) {
					return false;
				}
				a_process = _process;
				a_system = _system;
				return true;
			}

		private:
			void Run() noexcept
			{
				for (;;) {
					ProcessMemorySnapshot process{};
					SystemMemorySnapshot  system{};
					const bool            processOk = CaptureProcessMemory(process);
					const bool            systemOk = CaptureSystemMemory(system);
					if (processOk && systemOk) {
						std::lock_guard snapshotLock(_snapshotMutex);
						_process = process;
						_system = system;
						_hasSnapshot = true;
					}

					std::unique_lock lock(_mutex);
					if (_condition.wait_for(lock, std::chrono::seconds(1), [this] { return _stopping; })) {
						return;
					}
				}
			}

			std::mutex              _mutex;
			std::condition_variable _condition;
			bool                    _stopping{ false };
			std::thread             _thread;

			std::mutex            _snapshotMutex;
			ProcessMemorySnapshot _process{};
			SystemMemorySnapshot  _system{};
			bool                  _hasSnapshot{ false };
		};

		TelemetryWorker& Worker()
		{
			static TelemetryWorker worker;
			return worker;
		}
	}

	bool CaptureProcessMemory(ProcessMemorySnapshot& a_snapshot) noexcept
	{
		PROCESS_MEMORY_COUNTERS_EX counters{};
		counters.cb = sizeof(counters);

		if (!::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
			return false;
		}

		a_snapshot.workingSetBytes = counters.WorkingSetSize;
		a_snapshot.peakWorkingSetBytes = counters.PeakWorkingSetSize;
		a_snapshot.privateUsageBytes = counters.PrivateUsage;
		a_snapshot.pagefileUsageBytes = counters.PagefileUsage;
		return true;
	}

	bool CaptureSystemMemory(SystemMemorySnapshot& a_snapshot) noexcept
	{
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);

		if (!::GlobalMemoryStatusEx(&status)) {
			return false;
		}

		a_snapshot.totalPhysicalBytes = status.ullTotalPhys;
		a_snapshot.availablePhysicalBytes = status.ullAvailPhys;
		a_snapshot.totalPageFileBytes = status.ullTotalPageFile;
		a_snapshot.availablePageFileBytes = status.ullAvailPageFile;
		a_snapshot.memoryLoadPercent = status.dwMemoryLoad;
		return true;
	}

	void StartTelemetryWorker()
	{
		Worker().Start();
	}

	void StopTelemetryWorker() noexcept
	{
		Worker().Stop();
	}

	bool GetLatestTelemetry(ProcessMemorySnapshot& a_process, SystemMemorySnapshot& a_system) noexcept
	{
		return Worker().Read(a_process, a_system);
	}
}
