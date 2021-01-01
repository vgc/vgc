vgc_add_library(ui
    VGC_DEPENDENCIES
        core
        geometry
        graphics

    HEADER_FILES
        api.h
        button.h
        colorpalette.h
        column.h
        event.h
        exceptions.h
        flex.h
        label.h
        lengthpolicy.h
        lengthtype.h
        mouseevent.h
        row.h
        widget.h

    CPP_FILES
        button.cpp
        colorpalette.cpp
        column.cpp
        event.cpp
        exceptions.cpp
        flex.cpp
        label.cpp
        mouseevent.cpp
        row.cpp
        widget.cpp

    RESOURCE_FILES
        stylesheets/default.vgcss
)

add_subdirectory(wraps)
add_subdirectory(tests)
