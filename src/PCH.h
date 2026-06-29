#pragma once

// CommonLibSSE-NG — pulls in all RE:: and SKSE:: APIs
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// Standard library
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Logging
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std::string_view_literals;

namespace logger = SKSE::log;
