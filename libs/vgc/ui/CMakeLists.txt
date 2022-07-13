vgc_add_library(ui
    THIRD_DEPENDENCIES
        ${VGC_QT_GUI}

    VGC_DEPENDENCIES
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
        logcategories.h
        modifierkey.h
        mouseevent.h
        row.h
        shortcut.h
        sizepolicy.h
        strings.h
        widget.h
        window.h

        internal/paintutil.h
        internal/qopenglengine.h

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
        logcategories.cpp
        mouseevent.cpp
        row.cpp
        strings.cpp
        widget.cpp
        window.cpp

        internal/paintutil.cpp
        internal/qopenglengine.cpp

    RESOURCE_FILES
        stylesheets/default.vgcss
)

add_subdirectory(wraps)
add_subdirectory(tests)
