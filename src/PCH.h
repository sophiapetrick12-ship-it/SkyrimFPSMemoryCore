#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#pragma warning(push)
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <memory>
#include <string>
#include <vector>

using namespace std::literals;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
}
