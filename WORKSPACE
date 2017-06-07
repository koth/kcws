# Uncomment and update the paths in these entries to build the Android demo.
#since support libraries are not published in Maven Central or jCenter, we'll have a local copy


new_http_archive(
    name = "boost",
    urls = [
            #"https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.bz2/download",
            "https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2",
    ],
    build_file = "BUILD.boost",
    type = "tar.bz2",
    strip_prefix = "boost_1_64_0/",
    sha256 = "7bcc5caace97baa948931d712ea5f37038dbb1c5d89b43ad4def4ed7cb683332",
)


new_http_archive(
   name="tf",
   url = "https://gitlab.com/yovnchine/tfrelates/raw/master/tf_dist_1.2.0_rc1_0604.zip",
   strip_prefix = "tf_dist/",
   sha256 = "269115820a2ea4b7260f2ff131ed47860809e3ff05da763704a004724cea9775",
   build_file="BUILD.tf_dist",
)


#new_local_repository(
#   name="tf",
#   path = "/e/code/tf_dist",
#   build_file="BUILD.tf_dist",
#)


http_archive(
    name = "protobuf",
    urls = [
          "http://bazel-mirror.storage.googleapis.com/github.com/google/protobuf/archive/2b7430d96aeff2bb624c8d52182ff5e4b9f7f18a.tar.gz",
          "https://github.com/google/protobuf/archive/2b7430d96aeff2bb624c8d52182ff5e4b9f7f18a.tar.gz",
    ],
    sha256 = "e5d3d4e227a0f7afb8745df049bbd4d55474b158ca5aaa2a0e31099af24be1d0",
    strip_prefix = "protobuf-2b7430d96aeff2bb624c8d52182ff5e4b9f7f18a",
)