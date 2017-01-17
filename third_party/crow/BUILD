licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "crow",
    hdrs = glob([
        "include/crow.h",
        "include/crow/*.h",
        "include/crow/*.hpp",
    ]),
    linkopts = [
        "-pthread",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@boost//:system",
    ],
)

cc_binary(
    name = "crow_ex",
    srcs = [
        "examples/example.cpp",
    ],
    deps = [
        ":crow",
    ],
)
