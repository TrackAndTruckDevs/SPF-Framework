# For MSVC, use static runtime linking (/MT instead of /MD) to avoid missing DLL errors (Win32 Error 126) on end-user machines.
if(MSVC)
    set(variables
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
    )
    foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
            string(REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
    endforeach()
endif()

# Define the name and author of your plugin
set(PLUGIN_NAME "MyPlugin")
set(RC_COMPANY "Track'n'Truck Devs")

# Create the plugin as a shared library (DLL)
add_library(${PLUGIN_NAME} SHARED
    "MyPlugin.cpp"
)

# Expose CMake project() values as C++ preprocessor macros
# so PLUGIN_NAME and PLUGIN_VERSION and PLUGIN_AUTHOR stay in sync automatically.
target_compile_definitions(${PLUGIN_NAME} PRIVATE
    PLUGIN_NAME="${PLUGIN_NAME}"
    PLUGIN_VERSION="${PROJECT_VERSION}"
    PLUGIN_AUTHOR="${RC_COMPANY}"
)

# # Windows version resource (.rc file)
# See https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource
if(WIN32 OR MINGW)
    set(RC_DESCRIPTION "${PLUGIN_NAME} - SPF Plugin")
    set(RC_INTERNAL_NAME "${PLUGIN_NAME}.dll")
    set(RC_FILENAME "${PLUGIN_NAME}.dll")
    set(RC_PRODUCT "${PLUGIN_NAME}")
    configure_file(
        "${CMAKE_SOURCE_DIR}/version.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
        @ONLY
    )
    target_sources(${PLUGIN_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
    )
endif()
# The plugin needs to link against the main SPF library
# to resolve the API function calls at runtime.
# This also automatically handles include directories.
target_link_libraries(${PLUGIN_NAME} PRIVATE SPF)

if(MINGW)
    target_link_options(${PLUGIN_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

# All plugins should go into a 'plugins' subfolder in the output directory
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
)

# --- Code Signing (reuses cmake/sign.cmake — see root CMakeLists.txt) ---
# Cert config: SPF_SIGN_CERT + SPF_SIGN_PASS in CMakeUserPresets.json.
include(${CMAKE_SOURCE_DIR}/cmake/sign.cmake)
spf_sign_target(${PLUGIN_NAME})

# --- Deployment ---
# Deploys to ATS and/or ETS2 game dirs via the shared macro.
include(deploy)
spf_deploy_plugin(${PLUGIN_NAME})
