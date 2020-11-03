vgc_add_library(ui
    VGC_DEPENDENCIES
        core
        geometry
        graphics

    HEADER_FILES
        api.h
        button.h
        colorpalette.h
        event.h
        label.h
        mouseevent.h
        widget.h

    CPP_FILES
        button.cpp
        colorpalette.cpp
        event.cpp
        label.cpp
        mouseevent.cpp
        widget.cpp
)

add_subdirectory(wraps)
add_subdirectory(tests)
