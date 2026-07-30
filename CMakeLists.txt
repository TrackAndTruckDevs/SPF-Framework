# Set the minimum required CMake version.
cmake_minimum_required(VERSION 4.4)

# Define our project.
project(SPF VERSION 1.2.0 LANGUAGES CXX)
add_compile_definitions(_AMD64_)

# --- C++ Standard & IWYU ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Global MSVC Runtime Configuration (/MT) ---
# This ensures that the framework and ALL its dependencies (cpr, zlib, md4c, etc.) 
# use static runtime linking. This avoids "Win32 Error 126" on machines without 
# VC++ Redistributables and silences "D9025: overriding /MT with /MD" warnings.
if(MSVC)
    # 1. Set the modern CMake abstraction for runtime library selection
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # 2. Force /MT in global compiler flags for older dependencies or manual logic
    # Also replace /EHs or /EHsc with /EHa to catch SEH exceptions without D9025 warnings
    set(variables
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL
    )
    foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
            string(REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
        if(${variable} MATCHES "/EHsc")
            string(REPLACE "/EHsc" "/EHa" ${variable} "${${variable}}")
        elseif(${variable} MATCHES "/EHs")
            string(REPLACE "/EHs" "/EHa" ${variable} "${${variable}}")
        endif()
    endforeach()

    # Add /utf-8 flag to avoid encoding issues.
    add_compile_options(/utf-8)
endif()
# --- End of Global MSVC Runtime Configuration ---

# --- MinGW Cross-Compilation Global Flags ---
if(MINGW)
    set(MINGW_STATIC_FLAGS "-static-libgcc;-static-libstdc++")
    set(MINGW_WINPTHREAD_FLAGS "-Wl,-Bstatic;-lwinpthread;-Wl,-Bdynamic")
endif()
# --- End of MinGW Cross-Compilation Flags ---

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# --- Dependency Management (FetchContent) ---
include(FetchContent)

# Declare zlib (madler) dependency to override zlib-ng used by curl
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1 # A recent, stable version with a correct CMakeLists.txt
)

# Declare Dear ImGui dependency
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking # Use the docking branch
)

# Declare MinHook dependency
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG v1.3.4 # Current stable version
    # --- Persistent fix for CMake warning ---
    # Replace the outdated cmake_minimum_required version with a current one.
    # This is executed every time after the dependency is downloaded.
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/minhook.CMakeLists.txt"
    "<SOURCE_DIR>/CMakeLists.txt"
)

# Declare {fmt} dependency
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.0.0 # Latest stable version
)

# Declare nlohmann/json dependency
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0 # Latest stable version
)

# Declare cpr (C++ Requests) dependency
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.14.1 # Latest stable version
)

# Declare md4c dependency
FetchContent_Declare(
    md4c
    GIT_REPOSITORY https://github.com/mity/md4c.git
    GIT_TAG release-0.5.3 # Latest stable release tag
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/md4c.CMakeLists.txt"
    "<SOURCE_DIR>/CMakeLists.txt"
)
# --- End of Dependency Declarations ---

# --- Global Dependency Options ---
# Explicitly disable tests and examples for all dependencies where applicable
# to ensure the leanest possible build.
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MD4C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# --- End of Global Dependency Options ---

# --- CPR/libcurl Configuration ---
# Force libcurl (a dependency of cpr) to be built as a static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# Force libcurl to use Windows native SSL/TLS (SChannel) instead of OpenSSL
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
# Disable curl's verbose logging unless we explicitly want it
set(CURL_DISABLE_VERBOSE_STRINGS ON CACHE BOOL "" FORCE)
# We don't need the curl executable, only the library
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
# We don't need libpsl for our use case, and it requires Meson to build
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
# Disable curl tests, examples, and manual pages to speed up the build
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)

set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)

# Further disable unused curl protocols to minimize build size
set(CURL_DISABLE_FTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_DICT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_FILE ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMB ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_MQTT ON CACHE BOOL "" FORCE)
# --- End of CPR/libcurl Configuration ---

# --- ImGui Backend Options ---
set(IMGUI_BUILD_WIN32_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_DX11_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_DX12_BACKEND ON CACHE BOOL "" FORCE)
# Disable backends we don't need
set(IMGUI_BUILD_OPENGL3_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_VULKAN_BACKEND OFF CACHE BOOL "" FORCE)
set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# --- FetchContent MakeAvailable ---
FetchContent_MakeAvailable(imgui minhook fmt json cpr md4c)

# --- ImGui Static Library ---
set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SRC})
# Make ImGui header files available to everyone linking with this library.
# Also add our custom configuration file.
target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui_config"
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# --- Font Headers ---
set(SPF_FONT_HEADERS
    "include/SPF/UI/Fonts/FontAwesome7.h"
    "include/SPF/UI/Fonts/FontAwesome7Brands.h"
    "include/SPF/UI/Fonts/NotoSansBold.h"
    "include/SPF/UI/Fonts/NotoSansBoldItalic.h"
    "include/SPF/UI/Fonts/NotoSansItalic.h"
    "include/SPF/UI/Fonts/NotoSansRegular.h"
    "include/SPF/UI/Fonts/NotoSansMedium.h"
    "include/SPF/UI/Fonts/NotoSansMediumItalic.h"
    "include/SPF/UI/Fonts/RobotoMonoRegular.h"
)

# --- Windows Resource File ---
set(SPF_PLATFORM_SOURCES)
if(WIN32 OR MINGW)
    configure_file(version.rc.in version.rc @ONLY)
    list(APPEND SPF_PLATFORM_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )
endif()

# --- Main Framework Library ---
add_library(SPF SHARED
    ${SPF_FONT_HEADERS}
    ${SPF_PLATFORM_SOURCES}
    "src/dllmain.cpp"
    "src/Core/Core.cpp"
    "src/Config/ConfigService.cpp"
    "src/Data/GameData/GameDataCameraService.cpp"
    "src/Data/GameData/GameObjectVehicleService.cpp"
    "src/Data/GameData/GameObjectFileSystemService.cpp"
    "src/Data/GameData/GameObjectSessionService.cpp"
    "src/Data/GameData/GameWorldService.cpp"
    "src/Data/GameData/ClimateService.cpp"
    "src/Data/GameData/Finders/CoreCameraDataFinder.cpp"
    "src/Data/GameData/Finders/FreeCameraDataFinder.cpp"
    "src/Data/GameData/Finders/InteriorCameraDataFinder.cpp"
    "src/Data/GameData/Finders/FovDataFinder.cpp"
    "src/Data/GameData/Finders/ViewportDataFinder.cpp"
    "src/Data/GameData/Finders/BehindCameraDataFinder.cpp"
    "src/Data/GameData/Finders/TopCameraDataFinder.cpp"
    "src/Data/GameData/Finders/PhotoCameraDataFinder.cpp"
    "src/Data/GameData/Finders/CabinCameraDataFinder.cpp"
    "src/Data/GameData/Finders/WindowCameraDataFinder.cpp"
    "src/Data/GameData/Finders/BumperCameraDataFinder.cpp"
    "src/Data/GameData/Finders/WheelCameraDataFinder.cpp"
    "src/Data/GameData/Finders/TVCameraDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraStateDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraAnimationDataFinder.cpp"
    "src/Data/GameData/Finders/ObjectVehicleManagerFinder.cpp"
    "src/Data/GameData/Finders/FileSystemDataFinder.cpp"
    "src/Data/GameData/Finders/SessionDataFinder.cpp"
    "src/Data/GameData/Finders/GameWorldDataFinder.cpp"
    "src/Data/GameData/Finders/ClimateDataFinder.cpp"
    "src/Events/EventManager.cpp"
    "src/Events/Proxies/WndProcEventProxy.cpp"
    "src/Telemetry/GameContext.cpp"
    "src/Telemetry/GameDataProcessor.cpp"
    "src/Telemetry/SCSTelemetryService.cpp"
    "src/Telemetry/TruckProcessor.cpp"
    "src/Telemetry/TrailerProcessor.cpp"
    "src/Telemetry/JobProcessor.cpp"
    "src/Telemetry/EventsProcessor.cpp"
    "src/Telemetry/ControlsProcessor.cpp"
    "src/Telemetry/GearboxProcessor.cpp"
    "src/Telemetry/ConfigAttributeReader.cpp"
    "src/Hooks/HookManager.cpp"
    "src/Hooks/BaseHook.cpp"
    "src/Hooks/User32Hook.cpp"
    "src/Hooks/D3D11Hook.cpp"
    "src/Hooks/D3D12Hook.cpp"
    "src/Hooks/OpenGLHook.cpp"
    "src/Hooks/DInput8Hook.cpp"
    "src/Hooks/XInputHook.cpp"
    "src/Hooks/GameLogHook.cpp"
    "src/Hooks/PluginHook.cpp"
    "src/Hooks/CameraHooks.cpp"
    "src/Hooks/GameTools/ScsNameResolver.cpp"
    "src/GameCamera/GameCameraManager.cpp"
    "src/GameCamera/GameCameraInterior.cpp"
    "src/GameCamera/GameCameraPhoto.cpp"
    "src/GameCamera/GameCameraBehind.cpp"
    "src/GameCamera/GameCameraTop.cpp"
    "src/GameCamera/GameCameraCabin.cpp"
    "src/GameCamera/GameCameraWindow.cpp"
    "src/GameCamera/GameCameraBumper.cpp"
    "src/GameCamera/GameCameraWheel.cpp"
    "src/GameCamera/GameCameraTV.cpp"
    "src/GameCamera/GameCameraFree.cpp"
    "src/GameCamera/GameCameraDebug.cpp"
    "src/GameCamera/GameCameraDebugState.cpp"
    "src/GameCamera/GameCameraDebugAnimation.cpp"
    "src/Logging/Logger.cpp"
    "src/Logging/LoggerFactory.cpp"
    "src/Logging/Sinks/ErrorReportSink.cpp"
    "src/Logging/Sinks/FileSink.cpp"
    "src/Logging/Sinks/LoggerWindowSink.cpp"
    "src/Modules/HandleManager.cpp"
    "src/Modules/GameLogEventManager.cpp"
    "src/Modules/PluginManager.cpp"
    "src/Modules/API/CameraApi.cpp"
    "src/Modules/API/VehicleApi.cpp"
    "src/Modules/API/ClimateApi.cpp"
    "src/Modules/API/GameWorldApi.cpp"
    "src/Modules/API/UIApi.cpp"
    "src/Modules/API/LocalizationApi.cpp"
    "src/Modules/API/EnvironmentApi.cpp"
    "src/Modules/API/LoggerApi.cpp"
    "src/Modules/API/ManifestApi.cpp"
    "src/Modules/API/ConfigApi.cpp"
    "src/Modules/API/FormattingApi.cpp"
    "src/Modules/API/KeyBindsApi.cpp"
    "src/Modules/API/TelemetryApi.cpp"
    "src/Modules/API/VirtualInputApi.cpp"
    "src/Modules/API/GameConsoleApi.cpp"
    "src/Modules/API/JsonReaderApi.cpp"
    "src/Modules/API/JsonWriterApi.cpp"
    "src/Modules/API/JsonIOApi.cpp"
    "src/Modules/API/HooksApi.cpp"
    "src/Modules/API/GameLogApi.cpp"
    "src/Modules/CommunicationManager.cpp"
    "src/Modules/PerformanceMonitor.cpp"
    "src/Handles/GameLogCallbackHandle.cpp"
    "src/Modules/KeyBindsManager.cpp"
    "src/Modules/KeyboardInput.cpp"
    "src/Modules/GamepadInput.cpp"
    "src/Modules/GamepadAxisInput.cpp"
    "src/Modules/MouseInput.cpp"
    "src/Modules/MouseAxisInput.cpp"
    "src/Modules/JoystickInput.cpp"
    "src/Modules/JoystickAxisInput.cpp"
    "src/Modules/ChordInput.cpp"
    "src/Localization/LocalizationManager.cpp"
    "src/Renderer/D3D11RendererImpl.cpp"
    "src/Renderer/D3D12RendererImpl.cpp"
    "src/Renderer/OpenGLRendererImpl.cpp"
    "src/Renderer/Renderer.cpp"
    "src/UI/BaseWindow.cpp"
    "src/UI/ImGuiInputConsumer.cpp"
    "src/UI/LoggerWindow.cpp"
    "src/UI/PluginsWindow.cpp"
    "src/UI/SettingsWindow.cpp"
    "src/UI/HooksWindow.cpp"
    "src/UI/TelemetryWindow.cpp"
    "src/UI/GameConsoleWindow.cpp"
    "src/UI/CameraWindow.cpp"
    "src/UI/InfoWindow.cpp"
    "src/UI/GameWorldWindow.cpp"
    "src/UI/ClimateWindow.cpp"
    "src/UI/MainWindow.cpp"
    "src/UI/NotificationWindow.cpp"
    "src/UI/UIManager.cpp"
    "src/UI/WelcomeWindow.cpp"
    "src/UI/Resources/spf_logo.cpp"
    "src/UI/MarkdownRenderer.cpp"
    "src/UI/UIStyle.cpp"
    "src/UI/UITypographyHelper.cpp"
    "src/UI/UIElements.cpp"
    "src/System/VirtualKeyMapping.cpp"
    "src/System/GamepadButtonMapping.cpp"
    "src/System/MouseButtonMapping.cpp"
    "src/System/JoystickButtonMapping.cpp"
    "src/System/PathManager.cpp"
    "src/System/EnvironmentManager.cpp"
    "src/System/ApiService.cpp"
    "src/Utils/PatternFinder.cpp"
    "src/Utils/FinderLog.cpp"
    "src/Utils/HashUtils.cpp"
    "src/Utils/SystemUtils.cpp"
    "src/Utils/AvMitigation.cpp"
    "src/GameConsole/GameConsole.cpp"
    "src/System/Keyboard.cpp"
    "src/Input/InputManager.cpp"
    "src/Input/SCS/SCSInputService.cpp"
    "src/Input/SCS/VirtualDevice.cpp"
)
# --- Target Properties ---
set_target_properties(SPF PROPERTIES OUTPUT_NAME "spf-framework")
if(MINGW)
    set_target_properties(SPF PROPERTIES PREFIX "")
endif()

# --- Anti-Virus Heuristic Mitigation ---
# Aggressive size optimization (/Os) can sometimes produce code patterns 
# that resemble packed malware or shellcode.
if(MSVC)
    target_compile_options(SPF PRIVATE "$<$<CONFIG:Release>:/Ot>")
endif()

# --- Linker Options ---
if(MSVC)
    target_link_options(SPF PRIVATE
        "/EXPORT:scs_telemetry_init"
        "/EXPORT:scs_telemetry_shutdown"
        "/EXPORT:scs_input_init"
        "/EXPORT:scs_input_shutdown"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(SPF PRIVATE
        "-Wl,/EXPORT:scs_telemetry_init"
        "-Wl,/EXPORT:scs_telemetry_shutdown"
        "-Wl,/EXPORT:scs_input_init"
        "-Wl,/EXPORT:scs_input_shutdown"
    )
elseif(MINGW)
    # MinGW cross-compiler (GCC/Clang in GNU linker mode)
    target_link_options(SPF PRIVATE
        "-Wl,--export-all-symbols"
        ${MINGW_STATIC_FLAGS}
        ${MINGW_WINPTHREAD_FLAGS}
    )
endif()
# --- End of Linker Options ---

# --- Include Directories ---
target_include_directories(SPF
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/scs-sdk/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_lib"
    "${md4c_SOURCE_DIR}/src"
)

# --- Linking Dependencies ---
# Separate Windows system libs: MSVC requires '.lib' names, MinGW/Linux requires '-l' flags
if(MSVC)
    set(WINDOWS_SYSTEM_LIBS d3d11 d3d12 dxgi dinput8 dxguid opengl32.lib Psapi.lib xinput.lib)
else()
    # MinGW / GCC / Clang (Linux cross-compiling)
    set(WINDOWS_SYSTEM_LIBS d3d11 d3d12 dxgi dinput8 dxguid opengl32 psapi xinput1_4 dwmapi d3dcompiler)
endif()
# --- End of Linking Dependencies ---

target_link_libraries(SPF PRIVATE
    md4c
    imgui
    minhook
    fmt::fmt
    nlohmann_json::nlohmann_json
    cpr::cpr
    ${WINDOWS_SYSTEM_LIBS}
)

# --- PLUGINS INCLUSION ---
add_subdirectory(plugins)

# --- Automatic Deployment ---
# Paths are set via CMakePresets.json / CMakeUserPresets.json:
#   ATS_PLUGINS_DIR   — American Truck Simulator  plugins dir
#   ETS2_PLUGINS_DIR  — Euro Truck Simulator 2      plugins dir
#   INSTALLER_STAGE_DIR — [dev] staging for release packaging
# Set them in your preset's cacheVariables, e.g.:
#   "cacheVariables": { "ATS_PLUGINS_DIR": "$env{ATS_PLUGINS_DIR}" }
# If a path is empty or doesn't exist — that step is skipped silently.

set(ATS_PLUGINS_DIR "" CACHE PATH "Path to ATS bin/win_x64/plugins (set in CMakeUserPresets.json)")
set(ETS2_PLUGINS_DIR "" CACHE PATH "Path to ETS2 bin/win_x64/plugins (set in CMakeUserPresets.json)")
set(INSTALLER_STAGE_DIR "" CACHE PATH "[Dev] Staging directory for installer packaging (set in CMakeUserPresets.json)")

message(STATUS "")
message(STATUS "  ==================== DEPLOY ====================")

# --- ATS ---
message(STATUS "")
message(STATUS "  [ATS]      ${ATS_PLUGINS_DIR}")
if(ATS_PLUGINS_DIR AND EXISTS "${ATS_PLUGINS_DIR}")
    message(STATUS "           ✅ deployed successfully")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SPF>" "${ATS_PLUGINS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/Localization" "${ATS_PLUGINS_DIR}/spfAssets/localization"
        COMMENT "Deploying to ATS: ${ATS_PLUGINS_DIR}"
    )
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ✅  ATS deploy OK"
    )
elseif(ATS_PLUGINS_DIR)
    message(STATUS "           ❌ directory not found")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ❌  ATS deploy NOT FOUND"
    )
else()
    message(STATUS "           ⚠️  no path set")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ⚠️  ATS skipped - no path set in preset"
    )
endif()

# --- ETS2 ---
message(STATUS "")
message(STATUS "  [ETS2]     ${ETS2_PLUGINS_DIR}")
if(ETS2_PLUGINS_DIR AND EXISTS "${ETS2_PLUGINS_DIR}")
    message(STATUS "           ✅ deployed successfully")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SPF>" "${ETS2_PLUGINS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/Localization" "${ETS2_PLUGINS_DIR}/spfAssets/localization"
        COMMENT "Deploying to ETS2: ${ETS2_PLUGINS_DIR}"
    )
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ✅  ETS2 deploy OK"
    )
elseif(ETS2_PLUGINS_DIR)
    message(STATUS "           ❌ directory not found")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ❌  ETS2 deploy NOT FOUND"
    )
else()
    message(STATUS "           ⚠️  no path set")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ⚠️  ETS2 skipped - no path set in preset"
    )
endif()

# --- Installer Stage (for maintainers / release automation only) ---
# Set INSTALLER_STAGE_DIR in your CMakeUserPresets.json to enable automatic
# staging of the compiled DLL + assets into a distribution folder.
# Regular contributors can leave it unset — this block is skipped entirely.
message(STATUS "")
message(STATUS "  [Installer] ${INSTALLER_STAGE_DIR}")
if(INSTALLER_STAGE_DIR AND EXISTS "${INSTALLER_STAGE_DIR}")
    message(STATUS "           ✅ staged successfully")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_STAGE_DIR}/spfAssets/localization"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_STAGE_DIR}/spfPlugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SPF>" "${INSTALLER_STAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/Localization" "${INSTALLER_STAGE_DIR}/spfAssets/localization"
        COMMENT "Staging files for installer at ${INSTALLER_STAGE_DIR}"
    )
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ✅  Installer stage OK"
    )
elseif(INSTALLER_STAGE_DIR)
    message(STATUS "           ❌ directory not found")
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ❌  Installer stage NOT FOUND"
    )
else()
    message(STATUS "           ⚠️  not configured (maintainer use only)")
endif()

message(STATUS "")
message(STATUS "  ==================== END DEPLOY ====================")
# --- End of Deployment ---

# Status messages for verification.
message(STATUS "")
message(STATUS "  ==================== CONFIGURATION ====================")
message(STATUS "")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Generator:  ${CMAKE_GENERATOR}")
if(CMAKE_BUILD_TYPE)
    message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
endif()
if(MSVC)
    message(STATUS "  Runtime:    MSVC static (/MT)")
elseif(MINGW)
    message(STATUS "  Runtime:    MinGW static (-static-libgcc -static-libstdc++)")
endif()
if(CMAKE_CROSSCOMPILING)
    message(STATUS "  Platform:   ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_PROCESSOR}) → Windows (x86_64)")
else()
    message(STATUS "  Platform:   Windows (${CMAKE_SYSTEM_PROCESSOR})")
endif()
message(STATUS "  C++ std:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Output:     ${CMAKE_BINARY_DIR}")
message(STATUS "  Include:    ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "  SCS SDK:    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/scs-sdk/include")
message(STATUS "")
message(STATUS "  Dependencies: ImGui, MinHook, fmt, nlohmann/json, cpr, md4c")
message(STATUS "  (all resolved via FetchContent; stb_image vendored)")
message(STATUS "")
message(STATUS "  ==================== END CONFIGURATION =================")


# --- clangd compile_commands.json (POST_BUILD) ---
# clangd (via .clangd's CompilationDatabase: build) looks for
# build/compile_commands.json. When using CMake presets the file
# ends up in build/<preset>/compile_commands.json instead.
# This POST_BUILD step copies it so clangd can find it.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    string(COMPARE NOTEQUAL "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/build" NON_DEFAULT_BUILD)
    if(NON_DEFAULT_BUILD)
        set(CLANGD_LINK "${CMAKE_SOURCE_DIR}/build/compile_commands.json")
        message(STATUS "  clangd:    synced after build → ${CLANGD_LINK}")
        add_custom_command(TARGET SPF POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${CLANGD_LINK}"
            COMMENT "  clangd:    synced compile_commands.json"
        )
    endif()
endif()


# --- Plugin API Interface Library ---
# This INTERFACE library allows external projects (e.g., plugin developers)
# to easily include SPF's public plugin API headers using FetchContent.
# Plugin developers will link against 'spf_plugin_api' to get the necessary includes.
# It points directly to the SPF_API directory to enforce strict isolation.
add_library(spf_plugin_api INTERFACE)
target_include_directories(spf_plugin_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/SPF/SPF_API)
