vgc_add_library(ui
    VGC_DEPENDENCIES
        core
        graphics

    HEADER_FILES
        api.h
        colorpalette.h
        event.h
        mouseevent.h
        widget.h

    CPP_FILES
        colorpalette.cpp
        event.cpp
        mouseevent.cpp
        widget.cpp
)

add_subdirectory(wraps)
add_subdirectory(tests)
