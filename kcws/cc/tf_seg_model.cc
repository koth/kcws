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

#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "base/base.h"
#include "kcws/cc/pos_tagger.h"
#include "kcws/cc/viterbi_decode.h"
#include "sentence_breaker.h"  // NOLINT
#include "tfmodel/tfmodel.h"
#include "utils/basic_string_util.h"
#include "utils/basic_vocab.h"

#include "tensorflow/core/framework/types.pb.h"
#include "tensorflow/core/public/session.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"

// python tools/freeze_graph.py --input_graph ../kcws/logs/graph.pbtxt
// --input_checkpoint ../kcws/logs/model-29082 --output_node_names
// "transitions,BatchMatMul_1"   --output_graph
// ../kcws/kcws/models/seg_model.pbtxt

DEFINE_string(TRANSITION_NODE_NAME, "transitions",
              "the transitions node in graph model");
DEFINE_string(SCORES_NODE_NAME, "Reshape_7",
              "the final emission  node in graph model");
DEFINE_string(INPUT_NODE_NAME, "input_placeholder",
              "the input placeholder  node in graph model");

namespace kcws {

struct FakeEmitInfo {
  bool needFake;
  int weights[4];
  float totalWeight;
  FakeEmitInfo() {
    needFake = false;
    weights[0] = weights[1] = weights[2] = weights[3] = 1;
    totalWeight = 4;
  }
};
class KcwsScanReporter : public AcScanner<UnicodeStr, int>::ScanReporter {
 public:
  KcwsScanReporter(const UnicodeStr& ustr) : sentence_(ustr) {
    emit_infos_.resize(sentence_.size());
  }
  bool callback(uint32_t pos, int& weight, size_t len) override {
    if (len == 1) {
      emit_infos_[pos].needFake = true;
      emit_infos_[pos].weights[0] += weight;
    } else {
      for (size_t i = 0; i < len; i++) {
        uint32_t p = pos - len + 1 + i;
        emit_infos_[p].needFake = true;
        emit_infos_[p].totalWeight += weight;
        if (i == 0) {
          emit_infos_[p].weights[1] += weight;
        } else if (i == (len - 1)) {
          emit_infos_[p].weights[3] += weight;
        } else {
          emit_infos_[p].weights[2] += weight;
        }
      }
    }
    return false;
  }
  void fakePredication(
      Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>,
                       Eigen::Aligned>& predictions,
      int sentenceIdx) {
    size_t slen = sentence_.size();
    for (size_t i = 0; i < slen; i++) {
      if (emit_infos_[i].needFake) {
        predictions(sentenceIdx, i, 0) =
            log(emit_infos_[i].weights[0] / emit_infos_[i].totalWeight);
        predictions(sentenceIdx, i, 1) =
            log(emit_infos_[i].weights[1] / emit_infos_[i].totalWeight);
        predictions(sentenceIdx, i, 2) =
            log(emit_infos_[i].weights[2] / emit_infos_[i].totalWeight);
        predictions(sentenceIdx, i, 3) =
            log(emit_infos_[i].weights[3] / emit_infos_[i].totalWeight);
      }
    }
  }

 private:
  const UnicodeStr& sentence_;
  std::vector<FakeEmitInfo> emit_infos_;
};

TfSegModel::TfSegModel()
    : max_sentence_len_(0), bp_(nullptr), scores_(nullptr) {}
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
  FILE* fp = fopen(path.c_str(), "r");
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
      fclose(fp);
      return false;
    }
    const std::string& word = terms[0];
    if ((word == std::string("</s>")) || (word == std::string("<UNK>"))) {
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
bool TfSegModel::loadUserDict(const std::string& userDictPath) {
  FILE* fp = fopen(userDictPath.c_str(), "r");
  if (fp == NULL) {
    VLOG(0) << "open file error:" << userDictPath;
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
      VLOG(0) << "line len expected 2, but got:" << nn;
      fclose(fp);
      return false;
    }
    const std::string& word = terms[0];
    if ((word == std::string("</s>")) || (word == std::string("<UNK>"))) {
      continue;
    }
    UnicodeStr ustr;
    CHECK(BasicStringUtil::u8tou16(word.c_str(), word.size(), ustr));
    int weight = atoi(terms[1].c_str());
    scanner_.pushNode(ustr, weight);
    tn += 1;
  }
  if (tn > 1) {
    scanner_.buildFailNode();
  }
  fclose(fp);
  return true;
}
void TfSegModel::SetPosTagger(PosTagger* tagger) { tagger_.reset(tagger); }
bool TfSegModel::LoadModel(const std::string& modelPath,
                           const std::string& vocabPath, int maxSentenceLen,
                           const std::string& userDictPath) {
  breaker_.reset(new SentenceBreaker(maxSentenceLen));
  model_.reset(new tf::TfModel());

  if (!model_->Load(modelPath)) {
    VLOG(0) << "Could not load model from: " << modelPath;
    return false;
  }
  max_sentence_len_ = maxSentenceLen;

  std::vector<tensorflow::Tensor> trans_tensors;
  std::vector<std::string> output_names({FLAGS_TRANSITION_NODE_NAME});

  if (!model_->Eval({}, output_names, trans_tensors)) {
    LOG(ERROR) << "Error during get trans tensors: ";
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
  if (!load_vocab(vocabPath, &vocab_)) {
    return false;
  }
  num_words_ = vocab_.size();
  VLOG(0) << "Total word :" << num_words_;
  scores_ = new float*[2];
  scores_[0] = new float[num_tags_];
  scores_[1] = new float[num_tags_];
  bp_ = new int*[maxSentenceLen];
  for (int i = 0; i < maxSentenceLen; i++) {
    bp_[i] = new int[num_tags_];
  }
  if (!userDictPath.empty()) {
    CHECK(loadUserDict(userDictPath))
        << "load user dict error from path:" << userDictPath;
  }
  return true;
}

bool TfSegModel::Segment(const std::vector<UnicodeStr>& sentences,
                         std::vector<std::vector<SegTok>>* pTopResults) {
  if (sentences.empty()) {
    return true;
  }
  // Create input tensor
  tensorflow::Tensor input_tensor(
      tensorflow::DT_INT32,
      tensorflow::TensorShape(
          {static_cast<long long>(sentences.size()), max_sentence_len_}));

  auto input_tensor_mapped = input_tensor.tensor<tensorflow::int32, 2>();

  std::vector<std::pair<std::string, tensorflow::Tensor>> input_tensors(
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
        // set it to UNK token
        input_tensor_mapped(k, i) = 1;
      } else {
        input_tensor_mapped(k, i) = it->second;
      }
    }
    for (; i < max_sentence_len_; i++) {
      input_tensor_mapped(k, i) = 0;
    }
  }

  std::vector<tensorflow::Tensor> output_tensors;
  std::vector<std::string> output_names({FLAGS_SCORES_NODE_NAME});

  if (!model_->Eval(input_tensors, output_names, output_tensors)) {
    LOG(ERROR) << "Error during inference: ";
    return false;
  }

  tensorflow::Tensor* output = &output_tensors[0];
  Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>, Eigen::Aligned>
      predictions = output->tensor<float, 3>();
  for (size_t k = 0; k < ns; k++) {
    const UnicodeStr& word = sentences[k];
    if (scanner_.NumItem() > 0) {
      // 启用用户自定义词典
      KcwsScanReporter report(word);
      scanner_.doScan(word, &report);
      report.fakePredication(predictions, k);
    }
    size_t nn = word.size();
    std::vector<int> resultTags;
    get_best_path(predictions, k, nn, transitions_, bp_, scores_, resultTags,
                  num_tags_);
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
                         std::vector<std::string>* pTopResults,
                         std::vector<std::string>* pTags) {
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
    const std::vector<SegTok>& toks = topResults[i];
    size_t nn = toks.size();
    std::vector<std::string> todo;
    for (size_t k = 0; k < nn; k++) {
      std::string str;
      BasicStringUtil::u16tou8(ustr.c_str() + toks[k].first, toks[k].second,
                               str);
      pTopResults->push_back(str);
      todo.push_back(str);
    }
    if (pTags != nullptr && tagger_) {
      std::vector<std::vector<std::string>> todos;
      todos.push_back(todo);
      std::vector<std::vector<std::string>> tags;
      CHECK(tagger_->Tag(todos, tags)) << "pos tagger error";
      CHECK_EQ(tags[0].size(), todo.size()) << "pos tagger size not match";
      for (auto t : tags[0]) {
        pTags->push_back(t);
      }
    }
  }
  return true;
}
}  // namespace kcws
