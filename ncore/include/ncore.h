// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#ifdef WIN32
#define NCORE_API __declspec(dllexport)
#else
#define NCORE_API __attribute__((visibility("default")))
#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

NCORE_API void ncore_init();

#ifdef __cplusplus
}
#endif
