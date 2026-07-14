# ---------------------------------------------------------------------------
# spf_deploy_plugin(PLUGIN_NAME)
#
# Deploys a plugin DLL, localization and data to ATS and/or ETS2 game dirs.
# Paths come from ATS_PLUGINS_DIR / ETS2_PLUGINS_DIR (set in presets).
# Skips silently when a path is empty or directory doesn't exist.
# ---------------------------------------------------------------------------
function(spf_deploy_plugin PLUGIN_NAME)

    # --- Deploy to ATS ---
    if(ATS_PLUGINS_DIR AND EXISTS "${ATS_PLUGINS_DIR}")
        set(DEPLOY_DIR "${ATS_PLUGINS_DIR}/spfPlugins/${PLUGIN_NAME}")

        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
            COMMENT "ATS: ensuring dir ${DEPLOY_DIR}"
        )
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${PLUGIN_NAME}>" "${DEPLOY_DIR}"
            COMMENT "ATS: deploying ${PLUGIN_NAME}.dll"
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/localization")
            add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/localization" "${DEPLOY_DIR}/localization"
                COMMENT "ATS: copying localization for ${PLUGIN_NAME}"
            )
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
            add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/data" "${DEPLOY_DIR}/data"
                COMMENT "ATS: copying data for ${PLUGIN_NAME}"
            )
        endif()
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ✅  ${PLUGIN_NAME}: [ATS] deploy OK"
    )
    elseif(ATS_PLUGINS_DIR)
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ❌  ${PLUGIN_NAME}: [ATS] NOT FOUND"
        )
    else()
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ⚠️  ${PLUGIN_NAME}: [ATS] skipped - no path"
        )
    endif()

    # --- Deploy to ETS2 ---
    if(ETS2_PLUGINS_DIR AND EXISTS "${ETS2_PLUGINS_DIR}")
        set(DEPLOY_DIR "${ETS2_PLUGINS_DIR}/spfPlugins/${PLUGIN_NAME}")

        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
            COMMENT "ETS2: ensuring dir ${DEPLOY_DIR}"
        )
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${PLUGIN_NAME}>" "${DEPLOY_DIR}"
            COMMENT "ETS2: deploying ${PLUGIN_NAME}.dll"
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/localization")
            add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/localization" "${DEPLOY_DIR}/localization"
                COMMENT "ETS2: copying localization for ${PLUGIN_NAME}"
            )
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
            add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/data" "${DEPLOY_DIR}/data"
                COMMENT "ETS2: copying data for ${PLUGIN_NAME}"
            )
        endif()
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "  ✅  ${PLUGIN_NAME}: [ETS2] deploy OK"
    )
    elseif(ETS2_PLUGINS_DIR)
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ❌  ${PLUGIN_NAME}: [ETS2] NOT FOUND"
        )
    else()
        add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ⚠️  ${PLUGIN_NAME}: [ETS2] skipped - no path"
        )
    endif()

endfunction()
