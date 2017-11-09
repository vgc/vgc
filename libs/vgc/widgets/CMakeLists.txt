vgc_add_library(widgets
    THIRD_DEPENDENCIES
        Qt5::Widgets

    VGC_DEPENDENCIES
        interpreter
        scene

    CPP_FILES
        console.cpp
        viewer.cpp
        widget.cpp
)
