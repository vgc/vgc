vgc_add_library(ui
    VGC_DEPENDENCIES
        core
        geometry
        graphics

    HEADER_FILES
        api.h
        button.h
        colorpalette.h
        columnlayout.h
        event.h
        exceptions.h
        flexlayout.h
        label.h
        mouseevent.h
        rowlayout.h
        widget.h

    CPP_FILES
        button.cpp
        colorpalette.cpp
        columnlayout.cpp
        event.cpp
        exceptions.cpp
        flexlayout.cpp
        label.cpp
        mouseevent.cpp
        rowlayout.cpp
        widget.cpp
)

add_subdirectory(wraps)
add_subdirectory(tests)
