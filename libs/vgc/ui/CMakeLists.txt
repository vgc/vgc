vgc_add_library(ui
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h

    CPP_FILES
        dummy.cpp

    RESOURCE_FILES
)

add_subdirectory(wraps)
add_subdirectory(tests)
