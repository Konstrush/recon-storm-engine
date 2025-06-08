
if (WIN32)
set(SYSTEM_DEPS
    "comctl32"
    "dbghelp"
    "winhttp"
)
else()
set(SYSTEM_DEPS "ffi")
endif()

STORM_SETUP(
    TARGET_NAME engine
    TYPE executable
    DEPENDENCIES

    # internal
    core
    diagnostics
    util
    window

    # external
    sentry-native
    ${SDL2_LIBRARIES}
    zlib

    # system
    ${SYSTEM_DEPS}
)
