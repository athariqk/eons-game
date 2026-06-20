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
