vgc_add_library(app
    THIRD_DEPENDENCIES
        ${VGC_QT_WIDGETS}

    VGC_DEPENDENCIES
        ui

    HEADER_FILES
        application.h
        logcategories.h
        mainwindow.h
        mainwidget.h
        nativemenubar.h

    CPP_FILES
        application.cpp
        logcategories.cpp
        mainwidget.cpp
        mainwindow.cpp
        nativemenubar.cpp
)
