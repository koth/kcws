licenses(["notice"])

package(default_visibility = ["//visibility:public"])

package_version = "0.3.4"

package_file = "glog-" + package_version + ".tar.gz"

package_dir = "glog-" + package_version

genrule(
    name = "glog-srcs",
    srcs = [
        package_file,
    ],
    outs = [
        "include/glog/log_severity.h",
        "include/glog/logging.h",
        "include/glog/raw_logging.h",
        "include/glog/stl_logging.h",
        "include/glog/vlog_is_on.h",
        "lib/libglog.a",
    ],
    cmd = "\n".join([
        "export INSTALL_DIR=$$(pwd)/$(@D)",
        "export TMP_DIR=$$(mktemp -d -t glog.XXXXX)",
        "mkdir -p $$TMP_DIR",
        "cp -R $(SRCS) $$TMP_DIR",
        "cd $$TMP_DIR",
        "tar xfz " + package_file,
        "cd " + package_dir,
        "CPPFLAGS=\"-D_GLIBCXX_USE_CXX11_ABI=0\" ./configure --prefix=$$INSTALL_DIR --enable-shared=no --with-pic=yes",
        "make install",
        "rm -rf $$TMP_DIR",
    ]),
)

cc_library(
    name = "glog-cxx",
    srcs = [
        "empty.cc",
        "include/glog/log_severity.h",
        "include/glog/raw_logging.h",
        "include/glog/stl_logging.h",
        "include/glog/vlog_is_on.h",
        "lib/libglog.a",
    ],
    hdrs = [
        "include/glog/logging.h",
    ],
    includes = [
        "include",
    ],
    # linkstatic = 1,
    deps = [
        "//third_party/gflags:gflags-cxx",
    ],
)

filegroup(
    name = "glog",
    srcs = [
        ":glog-cxx",
    ],
)
