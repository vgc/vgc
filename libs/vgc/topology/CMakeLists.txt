vgc_add_library(topology
    VGC_DEPENDENCIES
        geometry

    HEADER_FILES
        api.h
        cell.h
        dataobject.h
        edgegeometry.h
        exceptions.h
        facemesh.h
        inbetweenedge.h
        inbetweenface.h
        inbetweenvertex.h
        keyedge.h
        keyface.h
        keyvertex.h
        operations.h
        transform.h
        vac.h

        detail/operationsimpl.h

    CPP_FILES
        cell.cpp
        exceptions.cpp
        operations.cpp
        transform.cpp
        vac.cpp

        detail/operationsimpl.cpp
)
