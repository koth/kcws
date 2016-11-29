# Uncomment and update the paths in these entries to build the Android demo.
#since support libraries are not published in Maven Central or jCenter, we'll have a local copy


new_http_archive(
    name = "boost",
    url = "https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.bz2/download",
    build_file = "BUILD.boost",
    type = "tar.bz2",
    strip_prefix = "boost_1_61_0/",
    sha256 = "a547bd06c2fd9a71ba1d169d9cf0339da7ebf4753849a8f7d6fdb8feee99b640",
)

local_repository(
  name = "org_tensorflow",
  path = __workspace_dir__+"/../tensorflow/",
)

load('@org_tensorflow//tensorflow:workspace.bzl', 'tf_workspace')
tf_workspace( __workspace_dir__+"/../tensorflow/", "@org_tensorflow")

# Specify the minimum required Bazel version.
load("@org_tensorflow//tensorflow:tensorflow.bzl", "check_version")
check_version("0.2.0")


