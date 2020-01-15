vgc_add_library(ui
    VGC_DEPENDENCIES
        core
        graphics

    HEADER_FILES
        api.h
        widget.h

    CPP_FILES
        widget.cpp
)

add_subdirectory(wraps)
add_subdirectory(tests)
