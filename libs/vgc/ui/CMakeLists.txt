vgc_add_library(ui
    THIRD_DEPENDENCIES
        Qt5::Gui

    VGC_DEPENDENCIES
        core
        geometry
        graphics

    HEADER_FILES
        action.h
        api.h
        button.h
        colorpalette.h
        column.h
        cursor.h
        event.h
        exceptions.h
        flex.h
        key.h
        label.h
        lineedit.h
        modifierkey.h
        mouseevent.h
        row.h
        shortcut.h
        sizepolicy.h
        strings.h
        style.h
        styletoken.h
        widget.h

        internal/paintutil.h

    CPP_FILES
        action.cpp
        button.cpp
        colorpalette.cpp
        column.cpp
        cursor.cpp
        event.cpp
        exceptions.cpp
        flex.cpp
        label.cpp
        lineedit.cpp
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
