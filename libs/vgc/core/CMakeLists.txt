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
        flags.h
        format.h
        history.h
        innercore.h
        io.h
        logcategories.h
        logging.h
        object.h
        os.h
        parse.h
        paths.h
        performancelog.h
        preprocessor.h
        profile.h
        python.h
        random.h
        stopwatch.h
        stringid.h
        templateutil.h

        detail/containerutil.h
        detail/signal.h

    CPP_FILES
        algorithm.cpp
        color.cpp
        exceptions.cpp
        format.cpp
        history.cpp
        io.cpp
        logcategories.cpp
        logging.cpp
        object.cpp
        parse.cpp
        paths.cpp
        performancelog.cpp
        profile.cpp
        python.cpp
        random.cpp
        stopwatch.cpp
        stringid.cpp

        detail/signal.cpp

    NATVIS_FILES
        tools/vgccore.natvis

    COMPILE_DEFINITIONS
)

vgc_optional_compile_definition(core PUBLIC VGC_CORE_USE_32BIT_INT "Define vgc::Int/UInt as 32-bit integers (default is 64-bit)")
vgc_optional_compile_definition(core PRIVATE VGC_CORE_OBJECT_DEBUG "Enable debug mode for vgc::core::Object")

add_dependencies(vgc_core_lib vgc_version)

add_subdirectory(wraps)
add_subdirectory(tests)
