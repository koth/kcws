package(default_visibility = ["//visibility:public"])

cc_library(
  name="tf_seg_model",
  srcs=[
    "tf_seg_model.cc"
  ],
  hdrs=[
    "tf_seg_model.h"
  ],
  deps=[
   '//utils:basic_string_util',
   '//utils:basic_vocab',
   ':pos_tagger',
   ':sentence_breaker',
   ':ac_scanner',
   '@tf//:tensorflow',
   '@protobuf//:protobuf',
  ]
)

cc_library(
  name="pos_tagger",
  srcs=[
    "pos_tagger.cc"
  ],
  hdrs=[
    "pos_tagger.h"
  ],
  deps=[
   '//utils:basic_string_util',
   '//utils:basic_vocab',
   '//tfmodel:tfmodel',
   ':viterbi_decode',
   '@tf//:tensorflow',
   '@protobuf//:protobuf',
  ]
)

cc_library(
  name="viterbi_decode",
  srcs=[
    "viterbi_decode.cc"
  ],
  hdrs=[
    "viterbi_decode.h"
  ],
  deps=[
   '//utils:basic_string_util',
   '@tf//:tensorflow',
  ]
)

cc_library(
  name="sentence_breaker",
  srcs=[
    "sentence_breaker.cc"
  ],
  hdrs=[
    "sentence_breaker.h"
  ],
  copts=[
  "-Wno-writable-strings"
  ],
  deps=[
  '//base:base',
   '//utils:basic_string_util',
  ],
  linkstatic=1,
)

cc_binary(
    name = "test_breaker",
    srcs = [
        "test_breaker.cc",
    ],
    copts = [
        "-g",
        "-std=c++11",
    ],
    linkopts = [
        "-ldl",
        "-lpthread",
    ],
    deps = [
        ":sentence_breaker",
        "//base",
    ],
)

py_binary(
    name = "dump_vocab",
    srcs = ["dump_vocab.py"],
    data = ["//utils:w2v.so"],
    imports = ["../../utils"],
)

cc_binary(
    name = "test_seg",
    srcs = [
        "test_seg.cc",
    ],
    copts = [
        "-g",
        "-std=c++11",
    ],
    linkopts = [
        "-ldl",
        "-lpthread",
    ],
    deps = [
        ":tf_seg_model",
        "//base",
    ],
)

cc_binary(
    name = "gen_seg_eval",
    srcs = [
        "gen_seg_eval.cc",
    ],
    copts = [
        "-g",
        "-std=c++11",
    ],
    linkopts = [
        "-ldl",
        "-lpthread",
    ],
    deps = [
        ":tf_seg_model",
        "//base",
    ],
)

genrule(
    name = "demo_html_gen",
    srcs = ["demo.html"],
    outs = ["demo_html.h"],
    cmd = "xxd -i \"$<\"  >\"$@\"",
)

cc_library(
    name = "demo_html",
    srcs = ["demo_html.h"],
    data = [
        ":demo_html_gen",
    ],
)

cc_binary(
    name = "seg_backend_api",
    srcs = [
        "seg_backend_api.cc",
    ],
    linkopts = ["-ldl"],
    deps = [
        ":demo_html",
        ":tf_seg_model",
        "//base",
        "//third_party/crow",
        "//utils:jsonxx",
    ],
)

cc_library(
    name = "ac_scanner",
    srcs = [
        "ac_scanner.h",
    ],
    linkstatic = 1,
    deps = [
        "//base",
        "//utils:basic_string_util",
    ],
)

cc_binary(
    name = "test_ac_scanner",
    srcs = [
        "test_ac_scanner.cc",
    ],
    copts = [
        "-Wno-writable-strings",
    ],
    linkopts = ["-ldl","-pthread"],
    deps = [
        ":ac_scanner",
    ],
)
