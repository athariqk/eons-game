// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if __has_cpp_attribute( clang::lifetimebound )
#define NC_LIFETIME_BOUND [[clang::lifetimebound]]
#else
#define NC_LIFETIME_BOUND
#endif
