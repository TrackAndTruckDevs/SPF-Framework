# ---------------------------------------------------------------------------
# spf_sign_target(TARGET_NAME)
#
# Signs a built DLL using osslsigncode (preferred) or signtool (fallback).
# SPF_SIGN_CERT and SPF_SIGN_PASS must be set (CACHE or regular).
# Skips silently when signing is not configured.
# ---------------------------------------------------------------------------
function(spf_sign_target TARGET_NAME)

    if(SPF_SIGN_CERT AND EXISTS "${SPF_SIGN_CERT}")
        # Try osslsigncode first (works on Linux, WSL, macOS, and Windows if installed)
        find_program(_OSLSIGNCODE_EXECUTABLE osslsigncode)
        if(_OSLSIGNCODE_EXECUTABLE)
            message(STATUS "  [SIGN]     ${TARGET_NAME}: ✅ signing enabled (osslsigncode)")
            set(_SIGN_ARGS sign -pkcs12 "${SPF_SIGN_CERT}")
            if(SPF_SIGN_PASS)
                list(APPEND _SIGN_ARGS -pass "${SPF_SIGN_PASS}")
            endif()
            list(APPEND _SIGN_ARGS
                -in "$<TARGET_FILE:${TARGET_NAME}>"
                -out "$<TARGET_FILE:${TARGET_NAME}>_signed"
            )
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND "${_OSLSIGNCODE_EXECUTABLE}" ${_SIGN_ARGS}
                COMMAND ${CMAKE_COMMAND} -E rename
                    "$<TARGET_FILE:${TARGET_NAME}>_signed"
                    "$<TARGET_FILE:${TARGET_NAME}>"
                COMMENT "  ✅ signing: ${TARGET_NAME}.dll (Track'n'Truck Devs)"
            )
        else()
            # Fallback: try signtool from Windows SDK
            find_program(_SIGNTOOL_EXECUTABLE signtool
                HINTS
                    "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64"
                    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
                    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
            )
            if(_SIGNTOOL_EXECUTABLE)
                message(STATUS "  [SIGN]     ${TARGET_NAME}: ✅ signing enabled (signtool)")
                set(_SIGN_CMD "${_SIGNTOOL_EXECUTABLE}" sign
                    /f "${SPF_SIGN_CERT}"
                    /td sha256 /fd sha256
                )
                if(SPF_SIGN_PASS)
                    list(APPEND _SIGN_CMD /p "${SPF_SIGN_PASS}")
                endif()
                list(APPEND _SIGN_CMD "$<TARGET_FILE:${TARGET_NAME}>")
                add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${_SIGN_CMD}
                    COMMENT "  ✅ signing: ${TARGET_NAME}.dll (Track'n'Truck Devs)"
                )
            else()
                message(WARNING "  [SIGN]     ${TARGET_NAME}: ⚠️  no signing tool found (install osslsigncode or signtool) — DLL will NOT be signed")
            endif()
        endif()
    else()
        message(STATUS "  [SIGN]     ${TARGET_NAME}: ⚠️  no cert set — signing skipped")
    endif()

endfunction()
