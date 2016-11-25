
cc_library(
    name = "basic_string_util",
    srcs = [
        "basic_string_util.h",
    ],
    visibility = ["//visibility:public"],
)


cc_library(
	name='jsonxx',
	srcs=[
	'jsonxx.cc',
	'jsonxx.h',
	'json_util.h'],
	hdrs=[
	'jsonxx.h',
	'json_util.h'],
	visibility = ["//visibility:public"],
	linkstatic=1,
       alwayslink=1,

	)



cc_library(
	name='word2vec_vob',
	srcs=[
	'word2vec_vob.cc'],
	hdrs=[
	'vocab.h',
	'word2vec_vob.h',
	],
	deps=[
	':basic_string_util',
	'//base:base',
   # '@re2//:re2',
	],
	visibility = ["//visibility:public"],
	copts=['-g','-O3','-std=c++11'],
	)


cc_binary(name='w2v.so',
	srcs=glob([
		'py_word2vec_vob.cc',
	]),
       deps=[
        ':word2vec_vob',
        '//third_party/pybind11:pybind11',
        '//base:base',
       ],
       copts=['-std=c++11','-fPIC'],
	visibility=['//visibility:public'],
       linkshared = 1,
	)
	
cc_library(
	name='basic_vocab',
	hdrs=['basic_vocab.h',
	'vocab.h'],
	srcs=[
	'basic_vocab.cc',],
	deps=[
	':basic_string_util',
	'//base:base',
	],
	visibility = ["//visibility:public"],
	copts=['-g','-O3','-std=c++11'],
	)

