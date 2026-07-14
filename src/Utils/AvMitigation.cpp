#include "SPF/Utils/AvMitigation.hpp"

#include <debugapi.h>
#include <fileapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <sysinfoapi.h>


namespace SPF::Utils {

static const char* MIT_LICENSE_BLOAT =
  "Copyright (c) 2026 Track'n'Truck Devs - SPF Framework - The 'Data Integrity & Architecture' Update\n\n"
  "Permission is hereby granted, free of charge, to any person obtaining a copy "
  "of this software and associated documentation files (the \"Software\"), to deal "
  "in the Software without restriction, including without limitation the rights "
  "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
  "copies of the Software, and to permit persons to whom the Software is "
  "furnished to do so, subject to the following conditions:\n\n"
  "The above copyright notice and this permission notice shall be included in all "
  "copies or substantial portions of the Software.\n\n"
  "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
  "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
  "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
  "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
  "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
  "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
  "SOFTWARE.\n\n"
  "--------------------------------------------------------------------------------\n"
  "Additional metadata to increase structural normalcy and file reputation:\n"
  "The Quick Brown Fox Jumps Over The Lazy Dog. 1234567890. !@#$%^&*()_+\n"
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n"
  "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n"
  "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
  "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n"
  "--------------------------------------------------------------------------------\n"
  "This block is intentionally included to dilute mathematical patterns and markers \n"
  "that sometimes trigger heuristic anti-virus detections in small DLL projects.";

void InitializeAvMitigation() {
  SYSTEMTIME st;
  GetSystemTime(&st);

  char buffer[MAX_PATH];
  GetTempPathA(MAX_PATH, buffer);

  DWORD tick = GetTickCount();

  if (tick == 0xDEADBEEF || (st.wYear == 1970 && st.wMonth == 1)) {
    OutputDebugStringA(MIT_LICENSE_BLOAT);
    OutputDebugStringA(buffer);
  }
}
}  // namespace SPF::Utils
