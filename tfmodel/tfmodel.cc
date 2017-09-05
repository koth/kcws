/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  tfmodel.cc
 * Author:  Koth
 * Create Time: 2017-02-01 13:28:34
 * Description:
 *
 */
#include "tfmodel/tfmodel.h"

#include <fstream>

#include "base/base.h"
#include "utils/basic_string_util.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"

class IfstreamInputStream : public ::google::protobuf::io::CopyingInputStream {
 public:
  explicit IfstreamInputStream(const std::string& file_name)
      : ifs_(file_name.c_str(), std::ios::in | std::ios::binary) {}
  ~IfstreamInputStream() { ifs_.close(); }

  int Read(void* buffer, int size) {
    if (!ifs_) {
      return -1;
    }
    ifs_.read(static_cast<char*>(buffer), size);
    return ifs_.gcount();
  }

 private:
  std::ifstream ifs_;
};

bool PortableReadFileToProto(const std::string& file_name,
                             ::google::protobuf::MessageLite* proto) {
  ::google::protobuf::io::CopyingInputStreamAdaptor stream(
      new IfstreamInputStream(file_name));
  stream.SetOwnsCopyingStream(true);
  // TODO(jiayq): the following coded stream is for debugging purposes to allow
  // one to parse arbitrarily large messages for MessageLite. One most likely
  // doesn't want to put protobufs larger than 64MB on Android, so we should
  // eventually remove this and quit loud when a large protobuf is passed in.
  ::google::protobuf::io::CodedInputStream coded_stream(&stream);
  // Total bytes hard limit / warning limit are set to 1GB and 512MB
  // respectively.
  coded_stream.SetTotalBytesLimit(1024LL << 20, 512LL << 20);
  return proto->ParseFromCodedStream(&coded_stream);
}

namespace tf {
TfModel::~TfModel() = default;
bool TfModel::Load(const std::string& path) {
  tensorflow::SessionOptions options;
  tensorflow::ConfigProto& config = options.config;

  session_.reset(tensorflow::NewSession(options));
  tensorflow::GraphDef tensorflow_graph;
  VLOG(0) << "Reading file to proto: " << path;
  if (!PortableReadFileToProto(path.c_str(), &tensorflow_graph)) {
    VLOG(0) << "Load model error from:" << path;
    return false;
  }
  VLOG(0) << "Creating session.";
  tensorflow::Status s = session_->Create(tensorflow_graph);
  if (!s.ok()) {
    VLOG(0) << "Could not create Tensorflow Graph: " << s;
    return false;
  }
  // Clear the proto to save memory space.
  tensorflow_graph.Clear();
  VLOG(0) << "Tensorflow graph loaded from: " << path;
  return true;
}
bool TfModel::Eval(
    const std::vector<std::pair<std::string, tensorflow::Tensor> >&
        inputTensors,
    const std::vector<std::string>& outputNames,
    std::vector<tensorflow::Tensor>& outputTensors) {
  tensorflow::Status s =
      session_->Run(inputTensors, outputNames, {}, &outputTensors);
  if (!s.ok()) {
    LOG(ERROR) << "Error during inference: " << s;
    return false;
  }
  return true;
}

}  // namespace tf
