#pragma once

// === STL ===
#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// === External libraries ===
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <box2d/box2d.h>
#include <imgui.h>
#include <inicpp.h>

// === Engine core ===
#include <ncore/core/errors.h>
#include <ncore/core/random.h>
#include <ncore/core/vector.h>
#include <ncore/core/types.h>

// === Utilities ==
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>
#include <utils/logger/logger.h>
