# This script is called by PATCH_COMMAND in FetchContent.
# It checks if a patch needs to be applied and does so idempotently.
# It's designed to be cross-platform and robust.

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR not defined for apply_patch_if_needed.cmake script.")
endif()
if(NOT DEFINED PATCH_FILE)
    message(FATAL_ERROR "PATCH_FILE not defined for apply_patch_if_needed.cmake script.")
endif()
if(NOT DEFINED GIT_EXECUTABLE)
    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "git executable not found and not passed to apply_patch_if_needed.cmake script.")
    endif()
endif()

# Check if the patch can be applied cleanly.
execute_process(
    COMMAND ${GIT_EXECUTABLE} apply --check ${PATCH_FILE}
    WORKING_DIRECTORY ${SOURCE_DIR}
    RESULT_VARIABLE patch_check_result
    OUTPUT_QUIET
    ERROR_QUIET
)

if(${patch_check_result} EQUAL 0)
    message(STATUS "Applying patch: ${PATCH_FILE}")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} apply ${PATCH_FILE}
        WORKING_DIRECTORY ${SOURCE_DIR}
        RESULT_VARIABLE patch_apply_result
    )
    if(NOT ${patch_apply_result} EQUAL 0)
        message(FATAL_ERROR "Failed to apply patch ${PATCH_FILE}. 'git apply' exited with code ${patch_apply_result}")
    endif()
else()
    message(STATUS "Patch seems to be already applied, skipping: ${PATCH_FILE}")
endif()
