vgc_add_library(core
    THIRD_DEPENDENCIES
        ${PYTHON_LIBRARIES}

    VGC_DEPENDENCIES

    HEADER_FILES
        algorithm.h
        api.h
        assert.h
        color.h
        colors.h
        compiler.h
        dll.h
        doublearray.h
        epsilon.h
        io.h
        logging.h
        mat4d.h
        math.h
        object.h
        os.h
        python.h
        random.h
        resources.h
        signal.h
        stringid.h
        stringutil.h
        timer.h
        vec2d.h
        vec2darray.h

    CPP_FILES
        algorithm.cpp
        color.cpp
        doublearray.cpp
        io.cpp
        logging.cpp
        mat4d.cpp
        math.cpp
        object.cpp
        python.cpp
        random.cpp
        resources.cpp
        signal.cpp
        stringid.cpp
        stringutil.cpp
        timer.cpp
        vec2d.cpp
        vec2darray.cpp

    COMPILE_DEFINITIONS
        -DVGC_CORE_PYTHON_PATH_="${VGC_PYTHON_OUTPUT_DIRECTORY}"
        -DVGC_CORE_RESOURCES_PATH_="${VGC_RESOURCES_OUTPUT_DIRECTORY}"
)

add_subdirectory(wraps)
