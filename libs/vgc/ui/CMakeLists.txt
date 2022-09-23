vgc_add_library(ui
    THIRD_DEPENDENCIES
        ${VGC_QT_GUI}

    VGC_DEPENDENCIES
        graphics

    HEADER_FILES
        action.h
        api.h
        button.h
        buttongroup.h
        colorpalette.h
        column.h
        cursor.h
        event.h
        exceptions.h
        flex.h
        grid.h
        key.h
        label.h
        lineedit.h
        logcategories.h
        margins.h
        menu.h
        menubutton.h
        modifierkey.h
        mouseevent.h
        overlayarea.h
        plot2d.h
        popuplayer.h
        qtutil.h
        row.h
        shortcut.h
        sizepolicy.h
        strings.h
        widget.h
        window.h

        detail/paintutil.h
        detail/qopenglengine.h

    CPP_FILES
        action.cpp
        button.cpp
        buttongroup.cpp
        colorpalette.cpp
        column.cpp
        cursor.cpp
        event.cpp
        exceptions.cpp
        flex.cpp
        grid.cpp
        label.cpp
        lineedit.cpp
        logcategories.cpp
        margins.cpp
        menu.cpp
        menubutton.cpp
        mouseevent.cpp
        overlayarea.cpp
        plot2d.cpp
        popuplayer.cpp
        qtutil.cpp
        row.cpp
        strings.cpp
        widget.cpp
        window.cpp

        detail/paintutil.cpp
        detail/qopenglengine.cpp

    RESOURCE_FILES
        stylesheets/default.vgcss
)

add_subdirectory(wraps)
add_subdirectory(tests)
