vgc_add_library(app
    THIRD_DEPENDENCIES
        ${VGC_QT_WIDGETS}

    VGC_DEPENDENCIES
        tools

    CPP_HEADER_FILES
        api.h
        canvasapplication.h
        filemanager.h
        logcategories.h
        mainwindow.h
        mainwidget.h
        nativemenubar.h
        qtwidgetsapplication.h

    CPP_SOURCE_FILES
        canvasapplication.cpp
        filemanager.cpp
        logcategories.cpp
        mainwidget.cpp
        mainwindow.cpp
        nativemenubar.cpp
        qtwidgetsapplication.cpp
)
