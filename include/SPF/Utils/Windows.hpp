// SPF Windows SDK wrapper.
// Centralises NOMINMAX and WIN32_LEAN_AND_MEAN so that:
//   1. min/max macros never collide with std::min/std::max.
//   2. Compilation is slightly faster (fewer rarely-used API headers).
//   3. As a project header (priority 2 in .clang-format), it is always
//      ordered before angle-bracket system headers like <psapi.h>,
//      preventing "unknown type name 'WINBOOL'" in clangd/MinGW.

#pragma once

#ifndef _MSC_VER
#include <bits/os_defines.h>
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>  // IWYU pragma: export
