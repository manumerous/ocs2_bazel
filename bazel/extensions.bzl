"""Module extension exposing ocs2_ros2's non-registry third-party repos.

These libraries have no Bazel registry module at all (or only an unusable
RCR stub -- see the per-repo notes below), so they are fetched with
http_archive. Creating them from a module extension, rather than with
`use_repo_rule` directly in MODULE.bazel, lets downstream modules import
the *same* repo instances this module builds against:

    ocs2_deps = use_extension("@ocs2_ros2//bazel:extensions.bzl", "ocs2_deps")
    use_repo(ocs2_deps, "pinocchio", "cppad")

instead of declaring their own copies (which would give two separate
pinocchio/cppad builds in one binary -- ODR / duplicate-symbol trouble).

Every `build_file` / `patches` entry is a `Label(...)` so it resolves into
this repo regardless of which module ends up evaluating the extension.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


# CppAD ships include/cppad/configure.hpp.in, normally substituted by its
# autoconf/CMake build. There's no Bazel-side substitution step here, so we
# write the same already-substituted configure.hpp that's committed today in
# ocs2_thirdparty/include/cppad/configure.hpp (produced by a real CppAD build
# once) directly into the fetched archive via patch_cmds -- same technique
# registries use via their `overlay` files, just inlined since this is the
# only generated file CppAD needs.
_CPPAD_CONFIGURE_HPP = """\
# ifndef CPPAD_CONFIGURE_HPP
# define CPPAD_CONFIGURE_HPP
# define CPPAD_COMPILER_HAS_CONVERSION_WARN 1
# define CPPAD_DISABLE_SOME_MICROSOFT_COMPILER_WARNINGS 1
# if _MSC_VER
# pragma warning( disable : 4100 )
# pragma warning( disable : 4127 )
# endif
# undef CPPAD_DISABLE_SOME_MICROSOFT_COMPILER_WARNINGS
# if     _MSC_VER
# define    CPPAD_USE_CPLUSPLUS_2011 1
# else   //
# if         __cplusplus >= 201100
# define         CPPAD_USE_CPLUSPLUS_2011 1
# else       //
# define         CPPAD_USE_CPLUSPLUS_2011 0
# endif      //
# endif //
# define CPPAD_PACKAGE_STRING "cppad-20190200.5"
# define CPPAD_HAS_ADOLC 0
# define CPPAD_HAS_COLPACK 0
# define CPPAD_HAS_EIGEN 0
# define CPPAD_HAS_IPOPT 0
# define CPPAD_DEPRECATED
# define CPPAD_BOOSTVECTOR 0
# define CPPAD_CPPADVECTOR 1
# define CPPAD_STDVECTOR 0
# define CPPAD_EIGENVECTOR 0
# define CPPAD_HAS_GETTIMEOFDAY 1
# define CPPAD_TAPE_ADDR_TYPE unsigned int
# define CPPAD_TAPE_ID_TYPE unsigned int
# ifndef CPPAD_MAX_NUM_THREADS
# define CPPAD_MAX_NUM_THREADS 48
# endif
# define CPPAD_HAS_MKSTEMP 1
# define CPPAD_HAS_TMPNAM_S 0
# ifndef CPPAD_NULL
# if CPPAD_USE_CPLUSPLUS_2011
# define CPPAD_NULL     nullptr
# else
# define CPPAD_NULL     0
# endif
# endif
# endif
"""

# CppAD also ships include/cppad/local/is_pod.hpp.in, substituted the same
# way (and, per a diff against the committed ocs2_thirdparty vendored copy,
# with a fixed expansion that doesn't depend on any build option -- just a
# static list of C++ fundamental types).
_CPPAD_IS_POD_HPP = """\
# ifndef CPPAD_LOCAL_IS_POD_HPP
# define CPPAD_LOCAL_IS_POD_HPP
# include <cstddef>
namespace CppAD { namespace local {
template <class T> bool is_pod(void) { return false; }
    template <> inline bool is_pod<bool>(void) { return true; }
    template <> inline bool is_pod<char>(void) { return true; }
    template <> inline bool is_pod<wchar_t>(void) { return true; }
    template <> inline bool is_pod<float>(void) { return true; }
    template <> inline bool is_pod<double>(void) { return true; }
    template <> inline bool is_pod<long double>(void) { return true; }
    template <> inline bool is_pod<signed char>(void) { return true; }
    template <> inline bool is_pod<unsigned char>(void) { return true; }
    template <> inline bool is_pod<signed short>(void) { return true; }
    template <> inline bool is_pod<unsigned short>(void) { return true; }
    template <> inline bool is_pod<signed int>(void) { return true; }
    template <> inline bool is_pod<unsigned int>(void) { return true; }
    template <> inline bool is_pod<signed long>(void) { return true; }
    template <> inline bool is_pod<unsigned long>(void) { return true; }
# if CPPAD_USE_CPLUSPLUS_2011
    template <> inline bool is_pod<char16_t>(void) { return true; }
    template <> inline bool is_pod<char32_t>(void) { return true; }
    template <> inline bool is_pod<signed long long>(void) { return true; }
    template <> inline bool is_pod<unsigned long long>(void) { return true; }
# endif
} }
# endif
"""

def _ocs2_deps_impl(mctx):
    # CppAD -- vendored today under ocs2_thirdparty/include/cppad.
    http_archive(
        name = "cppad",
        build_file = Label("//third_party/cppad:BUILD.cppad.bazel"),
        patch_cmds = [
            "cat > include/cppad/configure.hpp << 'CPPAD_CONFIGURE_HPP_EOF'\n" +
            _CPPAD_CONFIGURE_HPP + "CPPAD_CONFIGURE_HPP_EOF",
            "cat > include/cppad/local/is_pod.hpp << 'CPPAD_IS_POD_HPP_EOF'\n" +
            _CPPAD_IS_POD_HPP + "CPPAD_IS_POD_HPP_EOF",
        ],
        sha256 = "3b89e2a88c4fc7e263c67787e4dde2d3be5a786f389d9c608d21a1acffcee16a",
        strip_prefix = "CppAD-20190200.5",
        url = "https://github.com/coin-or/CppAD/archive/refs/tags/20190200.5.tar.gz",
    )

    # CppADCodeGen -- vendored today under ocs2_thirdparty/include/cppad/cg.
    http_archive(
        name = "cppadcodegen",
        build_file = Label("//third_party/cppadcodegen:BUILD.cppadcodegen.bazel"),
        sha256 = "fcbdf33d6957cfdc5d59123ad63ce4763690a895a234fbf551956f9bdd872e2d",
        strip_prefix = "CppADCodeGen-f3680f154c32f269123e6eea1531bb9030ad40ba",
        url = "https://github.com/joaoleal/CppADCodeGen/archive/f3680f154c32f269123e6eea1531bb9030ad40ba.tar.gz",
    )

    # BLASFEO -- fetched today via FetchContent in ocs2_sqp/blasfeo_catkin.
    http_archive(
        name = "blasfeo",
        build_file = Label("//third_party/blasfeo:BUILD.blasfeo.bazel"),
        sha256 = "da6e64f0e32018e40c6a189f012c0c74ce555c2c7b7b049ea1b7d63ced280e3a",
        strip_prefix = "blasfeo-ae6e2d1dea015862a09990b95905038a756ffc7d",
        url = "https://github.com/giaf/blasfeo/archive/ae6e2d1dea015862a09990b95905038a756ffc7d.tar.gz",
    )

    # HPIPM -- fetched today via FetchContent in ocs2_sqp/hpipm_catkin.
    http_archive(
        name = "hpipm",
        build_file = Label("//third_party/hpipm:BUILD.hpipm.bazel"),
        sha256 = "8c58e462f696809adea2278f697c46553569358a17ae7bf919d53590b8ff6e4a",
        strip_prefix = "hpipm-255ffdf38d3a5e2c3285b29568ce65ae286e5faf",
        url = "https://github.com/giaf/hpipm/archive/255ffdf38d3a5e2c3285b29568ce65ae286e5faf.tar.gz",
    )

    # Pinocchio -- real upstream stack-of-tasks/pinocchio, replacing the
    # blocked RCR `pinocchio` module (see the note in MODULE.bazel where that
    # bazel_dep used to be). URL/sha256/strip_prefix/patches/patch_args match exactly
    # what was specified when this scope expansion was approved; the sha256
    # below is what Bazel's own fetch reports for this URL, which matches the
    # value given at the time. `build_file` (a real file, not inlined
    # `build_file_content`) is used instead, for consistency with every other
    # http_archive overlay in this extension (cppad/cppadcodegen/blasfeo/hpipm) --
    # a large cc_library definition is far more reviewable/maintainable as
    # its own file than as a giant embedded string here.
    #
    # NOTE: split into 4 separate single-hunk patch files rather than one
    # combined patch (which was the user-specified starting point). Bazel's
    # built-in `ctx.patch` patch parser (used for `http_archive.patches`) has
    # a real limitation with multiple new-file ("--- /dev/null") hunks in one
    # patch file -- it applies the first hunk fine but then fails the rest
    # with "old file name is not specified, new file name ... doesn't exist",
    # even though the exact same patch content applies cleanly with GNU
    # `patch -p1` standalone (verified directly). One patch file per
    # generated header sidesteps it.
    http_archive(
        name = "pinocchio",
        build_file = Label("//third_party/pinocchio:BUILD.pinocchio.bazel"),
        patch_args = ["-p1"],
        patches = [
            Label("//patches:pinocchio-generated-config-hpp.patch"),
            Label("//patches:pinocchio-generated-deprecated-hpp.patch"),
            Label("//patches:pinocchio-generated-warning-hpp.patch"),
            Label("//patches:pinocchio-generated-parsers-config-hpp.patch"),
        ],
        sha256 = "e9ca048fd6c6e3f9e25ca691c7517c6cfdeb5218f03621dbe15ddeb7de2f9a1a",
        strip_prefix = "pinocchio-4.0.0",
        urls = ["https://github.com/stack-of-tasks/pinocchio/archive/refs/tags/v4.0.0.tar.gz"],
    )

    # orocos_kdl + kdl_parser -- same situation as pinocchio above: RCR's own
    # `kdl_parser` module (removed from MODULE.bazel, where its bazel_dep used to be)
    # registers only an ament-bookkeeping stub, and its real dependency
    # `orocos_kdl` has no RCR *or* BCR module at all. Unlike pinocchio,
    # orocos_kdl turned out to be a small, plain, non-template-heavy C++
    # library (~60 files, no per-feature source subsets, no boost.mpl/
    # boost.fusion arity surprises) -- confirmed by actually reading its
    # real CMakeLists.txt before assuming that, per the plan for this scope
    # expansion.
    http_archive(
        name = "orocos_kdl",
        build_file = Label("//third_party/orocos_kdl:BUILD.orocos_kdl.bazel"),
        patch_args = ["-p1"],
        # src/config.h.in needs a real configure_file substitution
        # (KDL_VERSION_MAJOR/MINOR/PATCH, KDL_USE_NEW_TREE_INTERFACE) --
        # rendered by hand from the tag's own top-level CMakeLists.txt
        # (SET(KDL_VERSION 1.5.1), and the "modern default" branch of the
        # HAVE_STL_CONTAINER_INCOMPLETE_TYPES/KDL_USE_NEW_TREE_INTERFACE
        # logic) and added as a real new-file patch, same technique as
        # pinocchio's generated headers -- tried a `write_file` generated
        # hdr through strip_include_prefix/include_prefix first, but the
        # quoted `#include "config.h"` from tree.hpp couldn't find it there
        # (a real, patched-in file works reliably; a generated one interacting
        # with that include remapping did not, and wasn't worth chasing
        # further given a working alternative existed).
        patches = [Label("//patches:orocos-kdl-generated-config-h.patch")],
        sha256 = "5acb90acd82b10971717aca6c17874390762ecdaa3a8e4db04984ea1d4a2af9b",
        strip_prefix = "orocos_kinematics_dynamics-1.5.1/orocos_kdl",
        urls = ["https://github.com/orocos/orocos_kinematics_dynamics/archive/refs/tags/v1.5.1.tar.gz"],
    )

    # Real ros2-gbp/kdl_parser-release source (the exact same tarball RCR's
    # own stub `kdl_parser` module points at -- see its source.json -- just
    # with a real BUILD.bazel this time).
    http_archive(
        name = "kdl_parser",
        build_file = Label("//third_party/kdl_parser:BUILD.kdl_parser.bazel"),
        sha256 = "c88bb839fa0826566e2f8f5dee38112c40c2d36b34770d1619cda8861262af0c",
        strip_prefix = "kdl_parser-release-release-lyrical-kdl_parser-3.0.1-3",
        url = "https://github.com/ros2-gbp/kdl_parser-release/archive/refs/tags/release/lyrical/kdl_parser/3.0.1-3.tar.gz",
    )

    # ocs2_robotic_assets -- URDFs/meshes, external repo cloned by CI today.
    # Pinned to the HEAD of `main` as of 2026-09-20.
    http_archive(
        name = "ocs2_robotic_assets",
        build_file = Label("//third_party/ocs2_robotic_assets:BUILD.ocs2_robotic_assets.bazel"),
        sha256 = "f1a68716aaa8ee5e0fdb77b3ed4bd43b4ffe2acb9c38f12aae9ee9e048921318",
        strip_prefix = "ocs2_robotic_assets-b126d00d55f1e67905c3c1516995466df6873c5c",
        url = "https://github.com/leggedrobotics/ocs2_robotic_assets/archive/b126d00d55f1e67905c3c1516995466df6873c5c.tar.gz",
    )

    # Everything above is pinned by sha256, so the extension's result
    # depends only on this file -- no need to record it in lockfiles.
    return mctx.extension_metadata(reproducible = True)

ocs2_deps = module_extension(
    implementation = _ocs2_deps_impl,
    doc = "Fetches ocs2_ros2's http_archive dependencies (pinocchio, cppad, ...).",
)
