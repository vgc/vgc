add_test(
    NAME
        vgc_test_dom_document
    COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_document.py -v
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(vgc_test_dom_document PROPERTIES
    ENVIRONMENT
        PYTHONPATH=${VGC_PYTHON_OUTPUT_DIRECTORY}:$ENV{PYTHONPATH}
)

add_test(
    NAME
        vgc_test_dom_xmlformattingstyle
    COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_xmlformattingstyle.py -v
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(vgc_test_dom_xmlformattingstyle PROPERTIES
    ENVIRONMENT
        PYTHONPATH=${VGC_PYTHON_OUTPUT_DIRECTORY}:$ENV{PYTHONPATH}
)

# XXX TODO Simplify the above with a CMake macro consistent with vgc_wrap_library, e.g.:
#   vgc_test_library(dom
#       PYTHON_TESTS
#           test_document.py
#           test_document.py
#   )
#
