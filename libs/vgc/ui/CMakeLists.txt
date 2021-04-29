vgc_add_library(ui
    THIRD_DEPENDENCIES
        Qt5::Gui

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
        key.h
        label.h
        modifierkey.h
        mouseevent.h
        row.h
        sizepolicy.h
        strings.h
        style.h
        styletoken.h
        widget.h

        internal/paintutil.h

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
        strings.cpp
        style.cpp
        styletoken.cpp
        widget.cpp

        internal/paintutil.cpp

    RESOURCE_FILES
        stylesheets/default.vgcss
)

add_subdirectory(wraps)
add_subdirectory(tests)
