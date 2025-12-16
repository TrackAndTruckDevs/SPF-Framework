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

# All plugins should go into a 'plugins' subfolder in the output directory
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${PLUGIN_NAME}"
)

# --- Deployment ---
# This section is optional but convenient. It copies the final DLL
# to the game's actual plugin directory with the correct structure.
if(EXISTS "${GAME_PLUGINS_DIR}")
    # Define the final deployment directory for this specific plugin
    set(PLUGIN_DEPLOY_DIR "${GAME_PLUGINS_DIR}/spfPlugins/${PLUGIN_NAME}")

    # This command ensures the destination directory exists before we try to copy the file.
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DEPLOY_DIR}"
        COMMENT "Ensuring deployment directory exists for ${PLUGIN_NAME}"
    )

    # This command copies the compiled DLL to the correct final location.
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PLUGIN_NAME}>"
            "${PLUGIN_DEPLOY_DIR}"
        COMMENT "Deploying ${PLUGIN_NAME}.dll to ${PLUGIN_DEPLOY_DIR}"
    )

    # This command copies the localization files to the deployment directory.
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/localization"
            "${PLUGIN_DEPLOY_DIR}/localization"
        COMMENT "Deploying localization files for ${PLUGIN_NAME}"
    )
endif()
