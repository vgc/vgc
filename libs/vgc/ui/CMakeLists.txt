vgc_add_library(ui
    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        widget.h

    CPP_FILES
        widget.cpp
)

add_subdirectory(wraps)
add_subdirectory(tests)
