// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/types/typedb.h>
#include <ncore/utils/command_line_parser.h>
#include <ncore/utils/config.h>

#include "services/servicedb.h"

namespace nc {

/**
 * @brief A semantic version representation for the application.
 */
struct REFLECT NCAPI AppVersion {
    REFLECT int major         = 0;
    REFLECT int minor         = 0;
    REFLECT int patch         = 0;
    REFLECT String identifier = "n";
};

/**
 * @brief AppDesc can be used to initialize an app with the given specification.
 */
struct REFLECT NCAPI AppDesc {
    REFLECT String name;                     // Name of the application.
    REFLECT AppVersion version;              // Semantic version of the application.
    REFLECT String config_file;              // Relative path to the application's config file.
    REFLECT CommandLineParser* cmdline_args; // The command-line arguments provided to the application.
};

/**
 * @brief State of an Application object.
 */
struct NCAPI AppContext {
    AppDesc app_desc;          // Descriptor used to init this application.
    ConfFile config;           // Parsed application config (loaded once in Application::init).
    ServiceDB services;        // Registry of all registered engine services.
    bool is_running   = false; // Whether game loop is still looping.
    uint64_t ticks    = 0;     // Ticks since first run() frame.
    double delta_time = 0.0;
};

} // namespace nc
