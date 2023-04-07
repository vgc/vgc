vgc_add_library(topology
    VGC_DEPENDENCIES
        geometry

    CPP_HEADER_FILES
        api.h
        cell.h
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
        vac.h

        detail/operationsimpl.h

    CPP_SOURCE_FILES
        cell.cpp
        edgegeometry.cpp
        exceptions.cpp
        logcategories.cpp
        operations.cpp
        transform.cpp
        vac.cpp

        detail/operationsimpl.cpp
)
