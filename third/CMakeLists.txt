#add_subdirectory(Eigen) # Nothing to do.

# Google Test. We need gtest_force_shared_crt ON, see:
# https://github.com/vgc/vgc/pull/295
set(gtest_force_shared_crt ON CACHE BOOL
  "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
add_subdirectory(googletest)

# pybind11
set(PYBIND11_CPP_STANDARD -std=c++11)
add_subdirectory(pybind11)
