// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#ifdef WIN32
#ifdef ncore_EXPORTS
#define NCAPI __declspec( dllexport )
#else
#define NCAPI __declspec( dllimport )
#endif

#else
#define NCAPI __attribute__( ( visibility( "default" ) ) )
#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

NCAPI void ncore_init();

#ifdef __cplusplus
}
#endif
