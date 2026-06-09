#pragma once

#define GAME_VERSION_MAJOR 0
#define GAME_VERSION_MINOR 1
#define GAME_VERSION_PATCH 0
#define GAME_VERSION_IDENTIFIER "pre-alpha"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RADIAN_TO_DEGREE 57.2957795131

#define _AEON_FE_1(M, S, a) M(S, a)
#define _AEON_FE_2(M, S, a, ...) M(S, a) _AEON_FE_1(M, S, __VA_ARGS__)
#define _AEON_FE_3(M, S, a, ...) M(S, a) _AEON_FE_2(M, S, __VA_ARGS__)
#define _AEON_FE_4(M, S, a, ...) M(S, a) _AEON_FE_3(M, S, __VA_ARGS__)
#define _AEON_FE_5(M, S, a, ...) M(S, a) _AEON_FE_4(M, S, __VA_ARGS__)
#define _AEON_FE_6(M, S, a, ...) M(S, a) _AEON_FE_5(M, S, __VA_ARGS__)
#define _AEON_FE_7(M, S, a, ...) M(S, a) _AEON_FE_6(M, S, __VA_ARGS__)
#define _AEON_FE_8(M, S, a, ...) M(S, a) _AEON_FE_7(M, S, __VA_ARGS__)

#define _AEON_FE_SEL(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define AEON_FOR_EACH(M, S, ...)                                                                                       \
    _AEON_FE_SEL(__VA_ARGS__, _AEON_FE_8, _AEON_FE_7, _AEON_FE_6, _AEON_FE_5, _AEON_FE_4, _AEON_FE_3, _AEON_FE_2,      \
                 _AEON_FE_1)(M, S, __VA_ARGS__)
