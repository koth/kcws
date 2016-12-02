/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  tf_seg_model.h
 * Author:  Koth
 * Create Time: 2016-11-20 10:31:03
 * Description:
 *
 */
#ifndef KCWS_TF_SEG_MODEL_H_
#define KCWS_TF_SEG_MODEL_H_
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "utils/basic_string_util.h"
#include "utils/basic_vocab.h"

namespace tensorflow {
class Session;
}  // namespace tensorflow
namespace kcws {
typedef std::pair<size_t, size_t> SegTok;
class SentenceBreaker;
class TfSegModel {
 public:
  TfSegModel();
  virtual  ~TfSegModel();

  bool LoadModel(const std::string& modelPath,
                 const std::string& vocabPath,
                 int maxSentenceLen);
  bool Segment(const std::string& sentence,
               std::vector<std::string>* pTopResult);
  bool Segment(const std::vector<UnicodeStr>& sentences,
               std::vector<std::vector<SegTok>>* pTopKResults);

 private:
  std::unique_ptr<tensorflow::Session> session_;
  std::unordered_map<UnicodeCharT, int> vocab_;
  std::unique_ptr<SentenceBreaker> breaker_;
  int max_sentence_len_;
  int num_words_;
  int num_tags_;
  std::vector<std::vector<float>> transitions_;
  int** bp_;
  float** scores_;
};

}  // namespace kcws

#endif  // KCWS_TF_SEG_MODEL_H_