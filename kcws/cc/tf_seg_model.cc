/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  tf_seg_model.cc
 * Author:  Koth
 * Create Time: 2016-11-20 10:31:03
 * Description:
 *
 */

#include "tf_seg_model.h"  //NOLINT

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include <queue>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "sentence_breaker.h"  // NOLINT
#include "base/base.h"
#include "utils/basic_vocab.h"
#include "utils/basic_string_util.h"
#include "tensorflow/core/framework/types.pb.h"
#include "tensorflow/core/public/session.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"

// python tensorflow/python/tools/freeze_graph.py --input_graph ../kcws/logs/graph.pbtxt  --input_checkpoint ../kcws/logs/model-29082 --output_node_names "transitions,BatchMatMul_1"   --output_graph ../kcws/kcws/models/seg_model.pbtxt

DEFINE_string(TRANSITION_NODE_NAME, "transitions", "the transitions node in graph model");
DEFINE_string(SCORES_NODE_NAME, "Reshape_7", "the final emission  node in graph model");
DEFINE_string(INPUT_NODE_NAME, "input_placeholder", "the input placeholder  node in graph model");

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
namespace kcws {
TfSegModel::TfSegModel(): max_sentence_len_(0), bp_(nullptr) , scores_(nullptr) {}
TfSegModel::~TfSegModel() {
  if (scores_) {
    if (scores_[0]) {
      delete[] scores_[0];
    }
    if (scores_[1]) {
      delete[] scores_[1];
    }
    delete[] scores_;
  }
  if (bp_) {
    for (int i = 0; i < max_sentence_len_; i++) {
      delete[] bp_[i];
    }
    delete[] bp_;
  }
}

bool load_vocab(const std::string& path,
                std::unordered_map<UnicodeCharT, int>* pVocab) {
  FILE *fp = fopen(path.c_str(), "r");
  if (fp == NULL) {
    fprintf(stderr, "open file error:%s\n", path.c_str());
    return false;
  }
  char line[4096] = {0};
  int tn = 0;
  while (fgets(line, sizeof(line) - 1, fp)) {
    int nn = strlen(line);
    while (nn && (line[nn - 1] == '\n' || line[nn - 1] == '\r')) {
      nn -= 1;
    }
    if (nn <= 0) {
      continue;
    }
    std::vector<std::string> terms;
    BasicStringUtil::SplitString(line, nn, '\t', &terms);
    nn = terms.size();
    if (nn != 2) {
      fprintf(stderr, "line len not comformed to dimension:%s:%d\n", line, nn);
      return false;
    }
    const std::string& word = terms[0];
    if ((word == std::string("</s>")) ||
        (word == std::string("<unk>"))) {
      continue;
    }
    UnicodeStr ustr;
    CHECK(BasicStringUtil::u8tou16(word.c_str(), word.size(), ustr));
    CHECK_EQ(ustr.size(), 1) << word;
    if (pVocab->find(ustr[0]) != pVocab->end()) {
      fprintf(stderr, "duplicate word:%s\n", word.c_str());
      fclose(fp);
      return false;
    }
    int idx = atoi(terms[1].c_str());
    pVocab->insert(std::make_pair(ustr[0], idx));
    tn += 1;
  }
  fclose(fp);
  return true;
}

bool TfSegModel::LoadModel(const std::string& modelPath,
                           const std::string& vocabPath,
                           int maxSentenceLen) {
  scores_ = new float*[2];
  scores_[0] = new float[num_tags_];
  scores_[1] = new float[num_tags_];
  bp_ = new int*[maxSentenceLen];
  for (int i = 0; i < maxSentenceLen; i++) {
    bp_[i] = new int[num_tags_];
  }
  breaker_.reset(new SentenceBreaker(maxSentenceLen));
  VLOG(0) << "Loading Tensorflow.";

  VLOG(0) << "Making new SessionOptions.";
  tensorflow::SessionOptions options;
  tensorflow::ConfigProto& config = options.config;
  VLOG(0) << "Got config, " << config.device_count_size() << " devices";

  session_.reset(tensorflow::NewSession(options));
  VLOG(0) << "Session created.";
  tensorflow::GraphDef tensorflow_graph;
  VLOG(0) << "Graph created.";
  VLOG(0) << "Reading file to proto: " << modelPath;
  if (!PortableReadFileToProto(modelPath.c_str(), &tensorflow_graph)) {
    LOG(ERROR) << "Load model error from:" << modelPath;
    return false;
  }
  VLOG(0) << "Creating session.";
  tensorflow::Status s = session_->Create(tensorflow_graph);
  if (!s.ok()) {
    VLOG(0) << "Could not create Tensorflow Graph: " << s;
    return false;
  }
  max_sentence_len_ = maxSentenceLen;


  std::vector<tensorflow::Tensor> trans_tensors;
  std::vector<std::string> output_names(
  {FLAGS_TRANSITION_NODE_NAME});

  s =
    session_->Run({}, output_names, {}, &trans_tensors);
  VLOG(0) << "End computing.";

  if (!s.ok()) {
    LOG(ERROR) << "Error during get trans tensors: " << s;
    return false;
  }
  VLOG(0) << "Reading from layer " << output_names[0];
  tensorflow::Tensor* output = &trans_tensors[0];
  const Eigen::TensorMap<Eigen::Tensor<float, 1, Eigen::RowMajor>,
        Eigen::Aligned>& prediction = output->flat<float>();
  const int count = prediction.size();
  num_tags_ = static_cast<int>(std::sqrt(count) + 0.01);
  VLOG(0) << "got num tag:" << num_tags_;

  for (int i = 0; i < num_tags_; i++) {
    transitions_.push_back(std::vector<float>());
    std::vector<float>& vec = transitions_.back();
    for (int j = 0; j < num_tags_; j++) {
      vec.push_back(prediction(j + i * num_tags_));
    }
  }
  // Clear the proto to save memory space.
  tensorflow_graph.Clear();
  VLOG(0) << "Tensorflow graph loaded from: " << vocabPath;
  if (!load_vocab(vocabPath, &vocab_)) {
    return false;
  }
  num_words_ = vocab_.size();
  VLOG(0) << "Total word :" << num_words_;
  return true;
}


static int viterbi_decode(
  const Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>, Eigen::Aligned>& predictions,
  int sentenceIdx,
  int nn,
  const std::vector<std::vector<float>>& trans,
  int** bp,
  float** scores,
  int ntags) {
  for (int i = 0; i < ntags; i++) {
    scores[0][i] = predictions(sentenceIdx, 0, i);
  }
  for (int i = 1; i < nn; i++) {
    for (int  t = 0; t < ntags; t++) {
      float maxScore = -1e7;
      float emission = predictions(sentenceIdx, i, t);
      for (int prev = 0; prev < ntags; prev++) {
        float score = scores[(i - 1) % 2][prev] + trans[prev][t] + emission;
        if (score > maxScore) {
          maxScore = score;
          bp[i - 1][t] = prev;
        }
      }
      scores[i % 2][t] = maxScore;
    }
  }
  float maxScore = scores[(nn - 1) % 2][0];
  int ret = 0;
  for (int i = 1; i < ntags; i++) {
    if (scores[(nn - 1) % 2][i] > maxScore) {
      ret = i;
      maxScore = scores[(nn - 1) % 2][i];
    }
  }
  return ret;
}

static void get_best_path(
  const Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>, Eigen::Aligned>& predictions,
  int sentenceIdx,
  int nn,
  const std::vector<std::vector<float>>& trans,
  int** bp,
  float** scores,
  std::vector<int>& resultTags,
  int ntags) {
  // std::vector<std::vector<int>> bp(nn - 1);
  // for (int i = 1; i < nn; i++) {
  //   for (int t = 0; t < ntags; t++) {
  //     bp[i - 1].push_back(-1);
  //   }
  // }
  int lastTag = viterbi_decode(predictions, sentenceIdx, nn, trans, bp, scores, ntags);
  resultTags.push_back(lastTag);
  for (int i = nn - 2; i >= 0; i--) {
    int bpTag = bp[i][lastTag];
    resultTags.push_back(bpTag);
    lastTag = bpTag;
  }
}


bool TfSegModel::Segment(const std::vector<UnicodeStr>& sentences,
                         std::vector<std::vector<SegTok>>* pTopResults) {

  if (sentences.empty()) {
    return true;
  }
  // Create input tensor
  tensorflow::Tensor input_tensor(
    tensorflow::DT_INT32,
    tensorflow::TensorShape({static_cast<long long>(sentences.size()), max_sentence_len_}));

  auto input_tensor_mapped = input_tensor.tensor<tensorflow::int32, 2>();

  std::vector<std::pair<std::string, tensorflow::Tensor> > input_tensors(
  {{FLAGS_INPUT_NODE_NAME, input_tensor}});

  size_t ns = sentences.size();
  for (size_t k = 0; k < ns; k++) {
    const UnicodeStr& word = sentences[k];
    int nn = word.size();
    if (nn <= 0) {
      VLOG(0) << "zero length str";
      return false;
    }
    int i = 0;
    if (nn > max_sentence_len_) {
      nn = max_sentence_len_;
    }
    for (; i < nn; i++) {
      const UnicodeCharT& w = word[i];
      auto it = vocab_.find(w);
      if (it == vocab_.end()) {
        input_tensor_mapped(k, i) = 0;
      } else {
        input_tensor_mapped(k, i) = it->second;
      }
    }
    for (; i < max_sentence_len_; i++) {
      input_tensor_mapped(k, i) = 0;
    }
  }


  std::vector<tensorflow::Tensor> output_tensors;
  std::vector<std::string> output_names(
  {FLAGS_SCORES_NODE_NAME});

  tensorflow::Status s =
    session_->Run(input_tensors, output_names, {}, &output_tensors);
  // VLOG(0) << "End computing.";

  if (!s.ok()) {
    LOG(ERROR) << "Error during inference: " << s;
    return false;
  }

  // VLOG(0) << "Reading from layer " << output_names[0];
  tensorflow::Tensor* output = &output_tensors[0];
  const Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>,
        Eigen::Aligned>& predictions = output->tensor<float, 3>();
  for (size_t k = 0; k < ns; k++) {
    const UnicodeStr& word = sentences[k];
    size_t nn = word.size();
    std::vector<int> resultTags;
    get_best_path(predictions, k, nn, transitions_, bp_, scores_, resultTags, num_tags_);
    CHECK_EQ(nn, resultTags.size()) << "num tag should equals setence len";
    pTopResults->push_back(std::vector<SegTok>());
    std::vector<SegTok>& resEle = pTopResults->back();
    size_t start = 0;
    for (size_t j = 0; j < nn; j++) {
      switch (resultTags[nn - j - 1]) {
      case 0:
        if (start < j) {
          resEle.push_back(SegTok{start, j - start});
        }
        resEle.push_back(SegTok{j, 1});
        start = j + 1;
        break;
      case 1:
        if (start < j) {
          resEle.push_back(SegTok{start, j - start});
        }
        start = j;
        break;
      case 2:
        break;
      case 3:
        resEle.push_back(SegTok{start, j - start + 1});
        start = j + 1;
        break;
      default:
        VLOG(0) << "Unkonw tag:" << resultTags[nn - j - 1];
        break;
      }
    }
    if (start < nn) {
      resEle.push_back(SegTok{start, nn - start});
    }
  }
  return true;
}
bool TfSegModel::Segment(const std::string& sentence,
                         std::vector<std::string>* pTopResults) {
  if (sentence.empty()) {
    return true;
  }

  UnicodeStr ustr;
  if (!BasicStringUtil::u8tou16(sentence.c_str(), sentence.size(), ustr)) {
    return false;
  }
  std::vector<UnicodeStr> sentences;
  if (!breaker_->breakSentences(ustr, &sentences)) {
    return false;
  }

  std::vector<std::vector<SegTok>> topResults;
  if (!Segment(sentences, &topResults)) {
    return false;
  }
  size_t ns = sentences.size();
  for (size_t i = 0; i < ns; i++) {
    const UnicodeStr& ustr = sentences[i];
    const std::vector<SegTok> & toks = topResults[i];
    size_t nn = toks.size();
    for (size_t k = 0; k < nn; k++) {
      std::string str;
      BasicStringUtil::u16tou8(ustr.c_str() + toks[k].first, toks[k].second, str);
      pTopResults->push_back(str);
    }
  }
  return true;
}
}  // namespace kcws

