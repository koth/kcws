/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  pos_tagger.cc
 * Author:  Koth
 * Create Time: 2017-02-01 14:30:22
 * Description:
 *
 */
#include "base/base.h"

#include "kcws/cc/pos_tagger.h"
#include "kcws/cc/viterbi_decode.h"
#include "tfmodel/tfmodel.h"

namespace kcws {

PosTagger::PosTagger() = default;
PosTagger::~PosTagger() = default;

// python tools/freeze_graph.py --input_graph pos_logs/graph.pbtxt  --input_checkpoint pos_logs/model-22176 --output_node_names "transitions,Reshape_9"   --output_graph  kcws/models/pos_model.pbtxt

static bool load_char_vocab(const std::string& path,
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
      fclose(fp);
      return false;
    }
    const std::string& word = terms[0];
    if ((word == std::string("</s>")) ||
        (word == std::string("<UNK>"))) {
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
static void convert_string_to_5_chars(const std::string& word,
                                      WordInfo& info,
                                      const std::unordered_map<UnicodeCharT, int>& charVocob) {
  UnicodeStr ustr;
  CHECK(BasicStringUtil::u8tou16(word.c_str(), word.size(), ustr));
  memset(&info.chars[0], 0, sizeof(info.chars));
  int nn = ustr.size();
  if (nn < 5) {
    for (int i = 0; i < nn; i++) {
      auto it = charVocob.find(ustr[i]);
      if (it == charVocob.end()) {
        // UNK
        info.chars[i] = 1;
      } else {
        info.chars[i] = it->second;
      }
    }
  }
  if (nn >= 5) {
    auto it = charVocob.find(ustr[nn - 1]);
    if (it == charVocob.end()) {
      // UNK
      info.chars[4] = 1;
    } else {
      info.chars[4] = it->second;
    }
  }
}
static bool load_word_vocab(const std::string& path,
                            std::unordered_map<std::string, WordInfo> * pVocab,
                            const std::unordered_map<UnicodeCharT, int>& charVocob) {
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
      fclose(fp);
      return false;
    }
    const std::string& word = terms[0];
    if ((word == std::string("</s>")) ||
        (word == std::string("<UNK>"))) {
      continue;
    }
    WordInfo info;
    convert_string_to_5_chars(word, info, charVocob);
    info.idx = atoi(terms[1].c_str());
    pVocab->insert(std::make_pair(word, info));
    tn += 1;
  }
  fclose(fp);
  return true;
}

static bool load_pos_vocab(const std::string& path,
                           std::unordered_map<int, std::string> * pVocab) {
  FILE *fp = fopen(path.c_str(), "r");
  if (fp == NULL) {
    fprintf(stderr, "open file error:%s\n", path.c_str());
    return false;
  }
  pVocab->clear();
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
    if ((word == std::string("</s>")) ||
        (word == std::string("<UNK>"))) {
      continue;
    }

    int idx = atoi(terms[1].c_str());
    pVocab->insert(std::make_pair(idx, word));
    tn += 1;
  }
  fclose(fp);
  return true;
}

void PosTagger::BuildWordInfo(const std::string& str, WordInfo& word) {
  auto it = word_vocab_.find(str);
  if (it == word_vocab_.end()) {
    convert_string_to_5_chars(str, word, char_vocab_);
    // UNK
    word.idx = 1;
  } else {
    word = it->second;
  }
}
bool PosTagger::LoadModel(const std::string& modelPath,
                          const std::string& wordVocabPath,
                          const std::string& charVocabPath,
                          const std::string& tagVocabPath,
                          int maxSentenceLen) {
  max_sentence_len_ = maxSentenceLen;
  model_.reset(new tf::TfModel());
  if (!model_->Load(modelPath)) {
    VLOG(0) << "load model error from:" << modelPath;
    return false;
  }
  if (!load_char_vocab(charVocabPath, &char_vocab_)) {
    VLOG(0) << "load char vocab error:" << charVocabPath;
    return false;
  }
  if (!load_word_vocab(wordVocabPath, &word_vocab_, char_vocab_)) {
    VLOG(0) << "load word vocab error:" << wordVocabPath;
    return false;
  }
  if (!load_pos_vocab(tagVocabPath, &tag_vocab_)) {
    VLOG(0) << "load tag vocab error:" << tagVocabPath;
    return false;
  }
  std::vector<tensorflow::Tensor> trans_tensors;
  std::vector<std::string> output_names(
  {"transitions"});

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
  scores_ = new float*[2];
  scores_[0] = new float[num_tags_];
  scores_[1] = new float[num_tags_];
  bp_ = new int*[maxSentenceLen];
  for (int i = 0; i < maxSentenceLen; i++) {
    bp_[i] = new int[num_tags_];
  }
  return true;
}
bool PosTagger::Tag(
  const std::vector<std::vector<std::string>>& sentences,
  std::vector<std::vector<std::string>>& tags) {
  if (sentences.empty()) {
    return true;
  }
  // Create input tensor
  tensorflow::Tensor input_tensor_w(
    tensorflow::DT_INT32,
    tensorflow::TensorShape({static_cast<long long>(sentences.size()), max_sentence_len_}));

  tensorflow::Tensor input_tensor_c(
    tensorflow::DT_INT32,
    tensorflow::TensorShape({static_cast<long long>(sentences.size()), max_sentence_len_ * 5}));

  auto input_tensor_w_mapped = input_tensor_w.tensor<tensorflow::int32, 2>();
  auto input_tensor_c_mapped = input_tensor_c.tensor<tensorflow::int32, 2>();
  std::vector<std::pair<std::string, tensorflow::Tensor> > input_tensors(
  {{"input_words", input_tensor_w}, {"input_chars", input_tensor_c}});

  size_t ns = sentences.size();
  for (size_t k = 0; k < ns; k++) {
    const std::vector<std::string>& sentence = sentences[k];
    int nn = sentence.size();
    if (nn <= 0) {
      VLOG(0) << "zero length str";
      return false;
    }
    int i = 0;
    if (nn > max_sentence_len_) {
      VLOG(0) << "too long a sentence";
      return false;
    }
    for (; i < nn; i++) {
      const std::string& w = sentence[i];
      WordInfo info;
      BuildWordInfo(w, info);
      input_tensor_w_mapped(k, i) = info.idx;
      input_tensor_c_mapped(k, i * 5) = info.chars[0];
      input_tensor_c_mapped(k, i * 5 + 1) = info.chars[1];
      input_tensor_c_mapped(k, i * 5 + 2) = info.chars[2];
      input_tensor_c_mapped(k, i * 5 + 3) = info.chars[3];
      input_tensor_c_mapped(k, i * 5 + 4) = info.chars[4];
    }
    for (; i < max_sentence_len_; i++) {
      input_tensor_w_mapped(k, i) = 0;
      input_tensor_c_mapped(k, i * 5) = 0;
      input_tensor_c_mapped(k, i * 5 + 1) = 0;
      input_tensor_c_mapped(k, i * 5 + 2) = 0;
      input_tensor_c_mapped(k, i * 5 + 3) = 0;
      input_tensor_c_mapped(k, i * 5 + 4) = 0;
    }
  }

  std::vector<tensorflow::Tensor> output_tensors;
  std::vector<std::string> output_names(
  {"Reshape_9"});


  if (!model_->Eval(input_tensors, output_names, output_tensors)) {
    LOG(ERROR) << "Error during inference: ";
    return false;
  }

  tensorflow::Tensor* output = &output_tensors[0];
  Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>,
        Eigen::Aligned> predictions = output->tensor<float, 3>();
  for (size_t k = 0; k < ns; k++) {
    const std::vector<std::string>& sentence = sentences[k];
    size_t nn = sentence.size();
    std::vector<int> resultTags;
    get_best_path(predictions, k, nn, transitions_, bp_, scores_, resultTags, num_tags_);
    CHECK_EQ(nn, resultTags.size()) << "num tag should equals setence len";
    tags.push_back(std::vector<std::string>());
    std::vector<std::string>& resEle = tags.back();
    for (size_t j = 0; j < nn; j++) {
      auto it = tag_vocab_.find(resultTags[nn - j - 1]);
      if (it == tag_vocab_.end()) {
        resEle.push_back("UNKNOWN");
      } else {
        resEle.push_back(it->second);
      }
    }
  }
  return true;
}

}  // namespace kcws
