vgc_add_library(vacomplex
    VGC_DEPENDENCIES
        geometry

    CPP_HEADER_FILES
        api.h
        cell.h
        complex.h
        dataobject.h
        edgegeometry.h
        exceptions.h
        inbetweenedge.h
        inbetweenface.h
        inbetweenvertex.h
        keycycle.h
        keyedge.h
        keyface.h
        keyhalfedge.h
        keyvertex.h
        logcategories.h
        operations.h
        transform.h

        detail/operationsimpl.h

    CPP_SOURCE_FILES
        cell.cpp
        complex.cpp
        edgegeometry.cpp
        exceptions.cpp
        inbetweenedge.cpp
        inbetweenface.cpp
        inbetweenvertex.cpp
        keycycle.cpp
        keyedge.cpp
        keyface.cpp
        keyhalfedge.cpp
        keyvertex.cpp
        logcategories.cpp
        operations.cpp
        transform.cpp

        detail/operationsimpl.cpp
)
