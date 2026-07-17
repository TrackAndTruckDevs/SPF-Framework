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
set(PLUGIN_NAME ExamplePlugin)

# Create the plugin as a shared library (DLL)
add_library(${PLUGIN_NAME} SHARED
    "ExamplePlugin.cpp"
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
# Paths come from ATS_PLUGINS_DIR / ETS2_PLUGINS_DIR in CMakeUserPresets.json.
include(deploy)
spf_deploy_plugin(${PLUGIN_NAME})

# --- Distribution for Installer (maintainer/release only) ---
# Only runs when INSTALLER_STAGE_DIR is set in CMakeUserPresets.json.
if(INSTALLER_STAGE_DIR)
    set(INSTALLER_PLUGIN_DIR "${INSTALLER_STAGE_DIR}/${PLUGIN_NAME}")

    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_PLUGIN_DIR}"
        COMMENT "Ensuring installer dist directory exists for ${PLUGIN_NAME}"
    )

    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PLUGIN_NAME}>"
        "${INSTALLER_PLUGIN_DIR}"
        COMMENT "Copying ${PLUGIN_NAME}.dll to installer dist"
    )

    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/localization"
        "${INSTALLER_PLUGIN_DIR}/localization"
        COMMENT "Copying localization files to installer dist"
    )

    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "${INSTALLER_PLUGIN_DIR}/data"
        COMMENT "Copying data dir to installer dist"
    )
endif()
