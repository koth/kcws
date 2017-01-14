/*
 * Copyright 2015 Rongall.com. All Rights Reserved.
 * =====================================================================================
 * Filename:  word2vec_vob.h
 * Description:   description
 * Author: Koth(Yaowen Chen)
 *
 */
#ifndef UTILS_WORD2VEC_VOB_H_
#define UTILS_WORD2VEC_VOB_H_
#include <vector>
#include <string>
#include <unordered_map>

#include "utils/vocab.h"

namespace utils {
struct WV;
class Word2vecVocab: public Vocab {
 public:
  enum OOV_OPT {
    USE_BLANK = 0,
    USE_OOV = 1,
    USE_RANDOM = 2,
    USE_ONE_RANDOM = 3,
  };
  Word2vecVocab(): f_dim_(0), avg_vals_(NULL), std_vals_(NULL), map_word_(false) {}
  virtual ~Word2vecVocab() {
    if (avg_vals_) {
      delete[] avg_vals_;
    }
    if (std_vals_) {
      delete[] std_vals_;
    }
    avg_vals_ = std_vals_ = NULL;
  }
  bool Load(const std::string& path) override;
  int GetVectorDim()const {
    return f_dim_;
  }
  void SetMapword(bool mapword);
  bool GetMapword();
  bool GetVector(const std::string& word, std::vector<float>** vec, OOV_OPT opt = USE_BLANK);
  std::vector<float> GetFeatureOrEmpty(const std::string& word);
  int GetWordIndex(const std::string& word) override;
  int GetTotalWord() override;
  bool DumpBasicVocab(const std::string& path);

 private:
  struct WV {
    std::vector<float> vect;
    int idx;
  };
  std::unordered_map<std::string, WV> f_map_;
  int f_dim_;
  float*  avg_vals_;
  float* std_vals_;
  std::vector<float> oov_feature_;
  bool map_word_;
};
}  // namespace utils
#endif  // UTILS_WORD2VEC_VOB_H_
