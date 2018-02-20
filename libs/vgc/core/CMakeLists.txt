vgc_add_library(core
    THIRD_DEPENDENCIES
        ${PYTHON_LIBRARIES}

    VGC_DEPENDENCIES

    HEADER_FILES
        algorithm.h
        api.h
        color.h
        colors.h
        export.h
        math.h
        object.h
        python.h
        random.h
        resources.h
        signal.h
        timer.h

    CPP_FILES
        algorithm.cpp
        color.cpp
        math.cpp
        object.cpp
        python.cpp
        random.cpp
        resources.cpp
        signal.cpp
        timer.cpp

    COMPILE_DEFINITIONS
        -DVGC_CORE_PYTHON_PATH_="${VGC_PYTHON_OUTPUT_DIRECTORY}"
        -DVGC_CORE_RESOURCES_PATH_="${VGC_RESOURCES_OUTPUT_DIRECTORY}"
)

add_subdirectory(wraps)
