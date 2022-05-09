vgc_add_library(core
    THIRD_DEPENDENCIES
        Python::Python
        pybind11::headers
        ${VGC_FMT}

    VGC_DEPENDENCIES

    HEADER_FILES
        algorithm.h
        api.h
        arithmetic.h
        array.h
        assert.h
        color.h
        colors.h
        compiler.h
        dll.h
        exceptions.h
        format.h
        innercore.h
        io.h
        logging.h
        mat3d.h
        mat3f.h
        mat4d.h
        mat4f.h
        object.h
        os.h
        parse.h
        paths.h
        performancelog.h
        pp.h
        python.h
        random.h
        stopwatch.h
        stringid.h
        templateutil.h
        vec2d.h
        vec2f.h

        internal/containerutil.h
        internal/signal.h

    CPP_FILES
        algorithm.cpp
        color.cpp
        exceptions.cpp
        format.cpp
        io.cpp
        logging.cpp
        mat4d.cpp
        mat4f.cpp
        mat3d.cpp
        mat3f.cpp
        object.cpp
        parse.cpp
        paths.cpp
        performancelog.cpp
        python.cpp
        random.cpp
        stopwatch.cpp
        stringid.cpp
        vec2d.cpp
        vec2f.cpp

        internal/signal.cpp

    NATVIS_FILES
        tools/vgccore.natvis

    COMPILE_DEFINITIONS
)

vgc_optional_compile_definition(core PUBLIC VGC_CORE_USE_32BIT_INT "Define vgc::Int/UInt as 32-bit integers (default is 64-bit)")
vgc_optional_compile_definition(core PRIVATE VGC_CORE_OBJECT_DEBUG "Enable debug mode for vgc::core::Object")

add_dependencies(vgc_core_lib vgc_version)

add_subdirectory(wraps)
add_subdirectory(tests)
