# Set the minimum required CMake version.
cmake_minimum_required(VERSION 3.16)

# Define our project.
project(SPF VERSION 1.0.0 LANGUAGES CXX)

# Enable generation of compile_commands.json, which is necessary for IWYU.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++20 standard for the entire project.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set /utf-8 flag for MSVC to avoid encoding issues in dependencies (e.g., fmt)
if(MSVC)
    add_compile_options(/utf-8)
endif()

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

# Declare md4c dependency (parser for imgui_md)
FetchContent_Declare(
    md4c
    GIT_REPOSITORY https://github.com/mity/md4c.git
    GIT_TAG release-0.5.2 # Latest stable release tag
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/md4c.CMakeLists.txt"
        "<SOURCE_DIR>/CMakeLists.txt"
)

# Declare imgui_md dependency
FetchContent_Declare(
    imgui_md
    GIT_REPOSITORY https://github.com/mekhontsev/imgui_md.git
    GIT_TAG main # Use main branch, as we will patch it
    PATCH_COMMAND ${CMAKE_COMMAND} -D SOURCE_DIR=<SOURCE_DIR> -D PATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/imgui_md.cpp.patch -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch_if_needed.cmake
)

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

# Disable md4c utility and imgui_md examples
set(MD4C_BUILD_MD2HTML OFF CACHE BOOL "" FORCE)
set(IMGUI_MD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Configure ImGui options before making it available
# We need backends for Win32 and DirectX 11
set(IMGUI_BUILD_WIN32_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_DX11_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_DX12_BACKEND ON CACHE BOOL "" FORCE)
# Disable backends we don't need
set(IMGUI_BUILD_OPENGL3_BACKEND ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_VULKAN_BACKEND OFF CACHE BOOL "" FORCE)
set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Do not build examples

# Make dependencies available for compilation
FetchContent_MakeAvailable(imgui minhook fmt json cpr md4c imgui_md)

# --- Creating targets for dependencies ---
# ImGui does not have its own CMakeLists that creates a library, so we define it here.
# This is an architecturally sound approach that encapsulates ImGui's build logic.
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
# Make ImGui header files available to everyone linking with this library.
# Also add our custom configuration file.
target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui_config"
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# md4c has its own CMakeLists.txt and creates the 'md4c' target.

# Create a library target for imgui_md
add_library(imgui_md STATIC
    ${imgui_md_SOURCE_DIR}/imgui_md.cpp
)
target_include_directories(imgui_md PUBLIC 
    ${imgui_md_SOURCE_DIR}
    ${md4c_SOURCE_DIR}/src # Add md4c's include directory
)
target_link_libraries(imgui_md PRIVATE md4c imgui)

# main framework library.
add_library(SPF SHARED

    ${SPF_FONT_HEADERS}

    "src/dllmain.cpp"
    "src/Core/Core.cpp"
    "src/Config/ConfigService.cpp"
    "src/Data/GameData/GameDataCameraService.cpp"
    "src/Data/GameData/GameObjectVehicleService.cpp"
    "src/Data/GameData/Finders/CoreCameraDataFinder.cpp"
    "src/Data/GameData/Finders/FreeCameraDataFinder.cpp"
    "src/Data/GameData/Finders/InteriorCameraDataFinder.cpp"
    "src/Data/GameData/Finders/FovDataFinder.cpp"
    "src/Data/GameData/Finders/ViewportDataFinder.cpp"
    "src/Data/GameData/Finders/BehindCameraDataFinder.cpp"
    "src/Data/GameData/Finders/TopCameraDataFinder.cpp"
    "src/Data/GameData/Finders/CabinCameraDataFinder.cpp"
    "src/Data/GameData/Finders/WindowCameraDataFinder.cpp"
    "src/Data/GameData/Finders/BumperCameraDataFinder.cpp"
    "src/Data/GameData/Finders/WheelCameraDataFinder.cpp"
    "src/Data/GameData/Finders/TVCameraDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraStateDataFinder.cpp"
    "src/Data/GameData/Finders/DebugCameraAnimationDataFinder.cpp"
    "src/Data/GameData/Finders/ObjectVehicleManagerFinder.cpp"
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

    "src/GameCamera/GameCameraManager.cpp"
    "src/GameCamera/GameCameraInterior.cpp"
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
    "src/Logging/Sinks/FileSink.cpp"
    "src/Logging/Sinks/LoggerWindowSink.cpp"
    "src/Modules/HandleManager.cpp"
    "src/Modules/GameLogEventManager.cpp"
    "src/Modules/PluginManager.cpp"
    "src/Modules/API/CameraApi.cpp"
    "src/Modules/API/UIApi.cpp"
    "src/Modules/API/LocalizationApi.cpp"
    "src/Modules/API/LoggerApi.cpp"
    "src/Modules/API/ManifestApi.cpp"
    "src/Modules/API/ConfigApi.cpp"
    "src/Modules/API/FormattingApi.cpp"
    "src/Modules/API/KeyBindsApi.cpp"
    "src/Modules/API/TelemetryApi.cpp"
    "src/Modules/API/VirtualInputApi.cpp"
    "src/Modules/API/GameConsoleApi.cpp"
    "src/Modules/API/JsonReaderApi.cpp"
    "src/Modules/API/HooksApi.cpp"
    "src/Modules/API/GameLogApi.cpp"
    "src/Modules/UpdateManager.cpp"
    "src/Modules/PerformanceMonitor.cpp"
    "src/Handles/GameLogCallbackHandle.cpp"
    "src/Modules/KeyBindsManager.cpp"
    "src/Modules/KeyboardInput.cpp"
    "src/Modules/GamepadInput.cpp"
    "src/Modules/MouseInput.cpp"
    "src/Modules/JoystickInput.cpp"
    
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
    "src/UI/MainWindow.cpp"
    "src/UI/UIManager.cpp"
    "src/UI/MarkdownRenderer.cpp"
    "src/UI/UIStyle.cpp"
    "src/UI/UITypographyHelper.cpp"
    "src/UI/UIElements.cpp"
    "src/System/VirtualKeyMapping.cpp"
    "src/System/GamepadButtonMapping.cpp"
    "src/System/MouseButtonMapping.cpp"
    "src/System/JoystickButtonMapping.cpp"
    "src/System/PathManager.cpp"
    "src/System/ApiService.cpp"
    "src/Utils/PatternFinder.cpp"
    "src/GameConsole/GameConsole.cpp"
    "src/System/Keyboard.cpp"
    "src/Input/InputManager.cpp"
    "src/Input/SCS/SCSInputService.cpp"
    "src/Input/SCS/VirtualDevice.cpp"
)

# Specify the correct output file name expected by the game.
set_target_properties(SPF PROPERTIES OUTPUT_NAME "spf-framework")

# Explicitly tell the linker which functions to export.
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
endif()

# Add include directories.
target_include_directories(SPF
    # Our public headers (<SPF/SomeFile.hpp>)
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    # Internal SDK dependency, needed for compilation
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/scs-sdk/include"
)

# --- Linking Dependencies ---
# Link compiled libraries to our project.
# imgui - main ImGui library
# minhook - MinHook library
# fmt::fmt - {fmt} library
# nlohmann_json::nlohmann_json - nlohmann/json library
# d3d11, dxgi - Windows system libraries for DirectX
# dinput8, dxguid - system libraries for DirectInput
target_link_libraries(SPF PRIVATE imgui_md imgui minhook fmt::fmt nlohmann_json::nlohmann_json cpr::cpr d3d11 d3d12 dxgi dinput8 dxguid opengl32.lib Psapi.lib xinput.lib)

# --- PLUGINS INCLUSION ---
# Add the plugins directory to the build.
# CMake will recursively process CMakeLists.txt inside this directory.
add_subdirectory(plugins)
# --- END OF PLUGINS INCLUSION ---


# --- Automatic Deployment ---
# Define the path to the game's plugins folder.
# This path can be changed in cmake-gui or via the command line:
# cmake -D GAME_PLUGINS_DIR="C:/Path/to/your/game/plugins"
set(GAME_PLUGINS_DIR "E:/SteamLibrary/steamapps/common/American Truck Simulator/bin/win_x64/plugins" CACHE PATH "Path to the game's plugins directory")

# Check if the path exists and display a message.
if(EXISTS "${GAME_PLUGINS_DIR}")
    message(STATUS "Deployment directory set to: ${GAME_PLUGINS_DIR}")

    # Create a command that executes AFTER the SPF target is built.
    # It copies the built DLL to the game's plugins folder.
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SPF>"
            "${GAME_PLUGINS_DIR}"
        COMMENT "Deploying scs-telemetry.dll to ${GAME_PLUGINS_DIR}"
    )

    # Create a command for copying localization resources.
    # It copies the entire contents of the assets/Localization folder.
    add_custom_command(TARGET SPF POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/Localization"
            "${GAME_PLUGINS_DIR}/spfAssets/localization"
        COMMENT "Deploying localization files to ${GAME_PLUGINS_DIR}/spfAssets/localization"
    )

else()
    message(WARNING "Deployment directory NOT FOUND: ${GAME_PLUGINS_DIR}. DLL and assets will not be deployed.")
endif()
# --- End of Deployment ---



# Status messages for verification.
message(STATUS "CMake configured for SPF (C++20)")
message(STATUS "SPF public include path: ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "SCS SDK private include path: ${CMAKE_CURRENT_SOURCE_DIR}/vendor/scs-sdk/include")
message(STATUS "Dependencies (ImGui, MinHook, fmt, nlohmann/json) configured via FetchContent.")

# --- Plugin API Interface Library ---
# This INTERFACE library allows external projects (e.g., plugin developers)
# to easily include SPF's public plugin API headers using FetchContent.
# Plugin developers will link against 'spf_plugin_api' to get the necessary includes.
# It points directly to the SPF_API directory to enforce strict isolation.
add_library(spf_plugin_api INTERFACE)
target_include_directories(spf_plugin_api
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SPF/SPF_API
)

# Generated Font Headers (Embedded in executable)
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