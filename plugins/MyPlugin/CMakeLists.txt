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

# Define the name of our plugin
set(PLUGIN_NAME MyPlugin)

# Create the plugin as a shared library (DLL)
add_library(${PLUGIN_NAME} SHARED
    "MyPlugin.cpp"
)

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

# --- Deployment ---
# Deploys to ATS and/or ETS2 game dirs via the shared macro.
include(deploy)
spf_deploy_plugin(${PLUGIN_NAME})
