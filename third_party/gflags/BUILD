licenses(["notice"])

package(default_visibility = ["//visibility:public"])


package_version = "2.0"

package_file =  "gflags-" + package_version + ".tar.gz"

package_dir = "gflags-" + package_version

genrule(
    name = "gflags-srcs",
    srcs = [
        package_file,
    ],
    outs = [
        "include/gflags/gflags.h",
        "include/gflags/gflags_completions.h",
        "include/gflags/gflags_declare.h",
        "include/google/gflags.h",
        "include/google/gflags_completions.h",
        "lib/libgflags.a",
        "lib/libgflags_nothreads.a",
    ],
    cmd = "\n".join([
        "export INSTALL_DIR=$$(pwd)/$(@D)",
        "export TMP_DIR=$$(mktemp -d -t gflags.XXXXX)",
        "mkdir -p $$TMP_DIR",
        "cp -R $(SRCS) $$TMP_DIR",
        "cd $$TMP_DIR",
        "tar xfz " + package_file,
        "cd " + package_dir,
        "CPPFLAGS=\"-D_GLIBCXX_USE_CXX11_ABI=0\" ./configure --prefix=$$INSTALL_DIR  --with-pic=yes --enable-shared=no",
        "make install",
        "rm -rf $$TMP_DIR",
    ]),
)

cc_library(
    name = "gflags-cxx",
    srcs = [
        "empty.cc",
        "include/gflags/gflags_declare.h",
        "lib/libgflags.a",
    ],
    hdrs = [
        "include/gflags/gflags.h",
    ],
    includes = [
        "include",
    ],
    # linkstatic = 1,
)

filegroup(
    name = "gflags",
    srcs = [
        ":gflags-cxx",
    ],
)
