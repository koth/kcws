cc_library(
    name = "basic_string_util",
    srcs = [
        "basic_string_util.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "jsonxx",
    srcs = [
        "json_util.h",
        "jsonxx.cc",
        "jsonxx.h",
    ],
    hdrs = [
        "json_util.h",
        "jsonxx.h",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    alwayslink = 1,
)

cc_library(
    name = "word2vec_vob",
    srcs = [
        "word2vec_vob.cc",
    ],
    hdrs = [
        "vocab.h",
        "word2vec_vob.h",
    ],
    copts = [
        "-g",
        "-O3",
        "-std=c++11",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":basic_string_util",
        "//base",
        # '@re2//:re2',
    ],
)

cc_binary(
    name = "w2v.so",
    srcs = glob([
        "py_word2vec_vob.cc",
    ]),
    copts = [
        "-std=c++11",
        "-fPIC",
    ],
    linkshared = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":word2vec_vob",
        "//base",
        "//third_party/pybind11",
    ],
)

cc_library(
    name = "basic_vocab",
    srcs = [
        "basic_vocab.cc",
    ],
    hdrs = [
        "basic_vocab.h",
        "vocab.h",
    ],
    copts = [
        "-g",
        "-O3",
        "-std=c++11",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":basic_string_util",
        "//base",
    ],
)
