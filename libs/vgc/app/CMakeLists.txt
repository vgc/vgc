vgc_add_library(app
    THIRD_DEPENDENCIES
        ${VGC_QT_WIDGETS}

    VGC_DEPENDENCIES
        ui

    CPP_HEADER_FILES
        application.h
        canvasapplication.h
        logcategories.h
        mainwindow.h
        mainwidget.h
        nativemenubar.h

    CPP_SOURCE_FILES
        application.cpp
        canvasapplication.cpp
        logcategories.cpp
        mainwidget.cpp
        mainwindow.cpp
        nativemenubar.cpp
)
