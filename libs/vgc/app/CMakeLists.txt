vgc_add_library(app
    THIRD_DEPENDENCIES
        ${VGC_QT_WIDGETS}

    VGC_DEPENDENCIES
        tools

    CPP_HEADER_FILES
        api.h
        canvasapplication.h
        logcategories.h
        mainwindow.h
        mainwidget.h
        nativemenubar.h
        qtwidgetsapplication.h

    CPP_SOURCE_FILES
        canvasapplication.cpp
        logcategories.cpp
        mainwidget.cpp
        mainwindow.cpp
        nativemenubar.cpp
        qtwidgetsapplication.cpp
)
