/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  pos_tagger.h
 * Author:  Koth
 * Create Time: 2017-02-01 14:02:35
 * Description:
 *
 */
#ifndef KCWS_CC_POS_TAGGER_H_
#define KCWS_CC_POS_TAGGER_H_
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "utils/basic_string_util.h"
#include "utils/basic_vocab.h"
namespace tf {
class TfModel;
}  // namespace tf
namespace kcws {
struct WordInfo {
  UnicodeCharT chars[5];
  int idx;
};
class PosTagger {
 public:
  PosTagger();
  virtual  ~PosTagger();

  bool LoadModel(const std::string& modelPath,
                 const std::string& wordVocabPath,
                 const std::string& charVocabPath,
                 const std::string& tagVocabPath,
                 int maxSentenceLen);
  bool Tag(const std::vector<std::vector<std::string>>& sentences,
           std::vector<std::vector<std::string>>& tags);
  void BuildWordInfo(const std::string& str, WordInfo& word);
 private:
  std::unique_ptr<tf::TfModel> model_;
  std::unordered_map<UnicodeCharT, int> char_vocab_;
  std::unordered_map<std::string, WordInfo> word_vocab_;
  std::unordered_map<int, std::string> tag_vocab_;
  int max_sentence_len_;
  int num_tags_;
  std::vector<std::vector<float>> transitions_;
  int** bp_;
  float** scores_;
};

}  // namespace kcws
#endif  // KCWS_CC_POS_TAGGER_H_
