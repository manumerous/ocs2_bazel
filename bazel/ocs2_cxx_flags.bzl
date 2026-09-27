"""Shared compile/link flags mirroring ocs2_core/cmake/ocs2_cxx_flags.cmake.

Every migrated OCS2 package's CMakeLists.txt either includes that file
directly (only ocs2_core does) or picks up its flags transitively through
`target_compile_options(ocs2_core PUBLIC ...)` propagating to consumers that
link it. Bazel's `copts`/`linkopts` don't propagate to dependents the way
CMake's PUBLIC compile options do, so this list is applied explicitly on
every migrated cc_library/cc_binary/cc_test instead.

DEVIATION FROM THE PLAN on the C++ standard: `set(CMAKE_CXX_STANDARD 14)`
in ocs2_cxx_flags.cmake is intentionally NOT mirrored here. OCS2_COPTS
instead applies `-std=c++23` to OCS2's own cc_library/cc_binary targets,
deliberately NOT as a repo-wide `--cxxopt` in .bazelrc -- a repo-wide
setting would also force every external dependency onto the same
standard, which isn't desirable for third-party code resolved from the
RCR/BCR graph. OCS2_TEST_COPTS and OCS2_ROS2_COPTS are kept as separate
names (rather than folding everything into OCS2_COPTS) purely so
cc_test/ROS2-bridge targets can still be pinned independently in the
future; today all three resolve to the same `-std=c++23`.
"""

# "-Wl,--no-as-needed" is a linker flag, not a compiler flag; kept out of
# OCS2_COPTS and applied only via OCS2_LINKOPTS.
OCS2_COPTS = [
    "-pthread",
    "-Wfatal-errors",
    "-fopenmp",
    "-std=c++23",
]

OCS2_TEST_COPTS = OCS2_COPTS[:-1] + ["-std=c++23"]

# Kept as its own name (rather than reusing OCS2_COPTS directly) for
# packages that bridge into ROS2 itself (ocs2_ros2_interfaces and the
# *_ros example binaries), in case the ROS2 integration layer ever needs
# a standard override independent of the solver/math core again.
OCS2_ROS2_COPTS = OCS2_COPTS[:-1] + ["-std=c++23"]

OCS2_LINKOPTS = [
    "-pthread",
    "-Wl,--no-as-needed",
    "-fopenmp",
]

# DEVIATION FROM THE PLAN: ocs2_cxx_flags.cmake sets `-DBOOST_ALL_DYN_LINK`
# because the original CMake build links against a prebuilt *dynamic*
# system libboost_log.so. Here, @boost.log (from BCR) is compiled from
# source as a plain cc_library (a static archive by default, built without
# that define). Boost.Log's headers pick one of two ABI-versioned inline
# namespaces depending on this macro (`v2_mt_posix` with the define,
# `v2s_mt_posix` -- note the "s" -- without it); defining it on our side
# while @boost.log itself was compiled without it produces exactly the
# "undefined reference to boost::log::v2_mt_posix::core::..." link errors
# this was verified against (the compiled archive only has the
# `v2s_mt_posix`-mangled symbols). So this define is dropped rather than
# carried over -- it must match whatever @boost.log itself was compiled
# with, not the original CMake build's linking strategy.
OCS2_DEFINES = []
