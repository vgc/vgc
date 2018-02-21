vgc_add_library(widgets
    THIRD_DEPENDENCIES
        Qt5::Widgets

    VGC_DEPENDENCIES
        core
        scene

    HEADER_FILES
        api.h
        colordialog.h
        colorselector.h
        console.h
        dialog.h
        mainwindow.h
        openglviewer.h
        qtutil.h

    CPP_FILES
        colordialog.cpp
        colorselector.cpp
        console.cpp
        dialog.cpp
        mainwindow.cpp
        openglviewer.cpp
        qtutil.cpp

    RESOURCE_FILES
        fonts/SourceCodePro-Black.ttf
        fonts/SourceCodePro-Bold.ttf
        fonts/SourceCodePro-ExtraLight.ttf
        fonts/SourceCodePro-Light.ttf
        fonts/SourceCodePro-Medium.ttf
        fonts/SourceCodePro-OFL.txt
        fonts/SourceCodePro-Regular.ttf
        fonts/SourceCodePro-Semibold.ttf
)
