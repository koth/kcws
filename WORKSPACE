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
   url = "http://ojsyioumh.bkt.clouddn.com/tf_dist_1.0.0alpha_3.zip",
   strip_prefix = "tf_dist/",
   sha256 = "91d607120d37ff2e3483922179611dc3894ae23d107f8f21cec7ac8b3c97fe25",
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
          "http://bazel-mirror.storage.googleapis.com/github.com/google/protobuf/archive/008b5a228b37c054f46ba478ccafa5e855cb16db.tar.gz",
          "https://github.com/google/protobuf/archive/008b5a228b37c054f46ba478ccafa5e855cb16db.tar.gz",
    ],
    sha256 = "2737ad055eb8a9bc63ed068e32c4ea280b62d8236578cb4d4120eb5543f759ab",
    strip_prefix = "protobuf-008b5a228b37c054f46ba478ccafa5e855cb16db",
)
