package(default_visibility = ["//visibility:public"])

cc_library(
  name="tfmodel",
  srcs=[
    "tfmodel.cc"
  ],
  hdrs=[
    "tfmodel.h"
  ],
  linkstatic=1,
  deps=[
   '//base:base',
   '//utils:basic_string_util',
   '@tf//:tensorflow',
  ]
)
