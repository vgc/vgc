vgc_add_library(widgets
    THIRD_DEPENDENCIES
        Qt5::Widgets

    VGC_DEPENDENCIES
        core
        scene

    CPP_FILES
        console.cpp
        openglviewer.cpp
        widget.cpp
)

add_subdirectory(fonts)
