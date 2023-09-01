vgc_add_library(vacomplex
    VGC_DEPENDENCIES
        geometry

    CPP_HEADER_FILES
        api.h
        cell.h
        celldata.h
        cellproperty.h
        complex.h
        exceptions.h
        inbetweenedge.h
        inbetweenface.h
        inbetweenvertex.h
        keycycle.h
        keyedge.h
        keyedgedata.h
        keyface.h
        keyhalfedge.h
        keyvertex.h
        logcategories.h
        operations.h
        transform.h

        detail/operationsimpl.h

    CPP_SOURCE_FILES
        cell.cpp
        celldata.cpp
        cellproperty.cpp
        complex.cpp
        exceptions.cpp
        inbetweenedge.cpp
        inbetweenface.cpp
        inbetweenvertex.cpp
        keycycle.cpp
        keyedge.cpp
        keyedgedata.cpp
        keyface.cpp
        keyhalfedge.cpp
        keyvertex.cpp
        logcategories.cpp
        operations.cpp
        transform.cpp

        detail/operationsimpl.cpp
)
