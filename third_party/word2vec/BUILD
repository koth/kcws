licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "word2vec",
    srcs = [
        "word2vec.c",
    ],
    linkopts = [
        "-pthread",
    ],
)

cc_binary(
    name = "distance",
    srcs = [
        "distance.cc",
    ],
    deps = [
        "//utils:basic_string_util",
    ],
)
