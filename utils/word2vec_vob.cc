/*
 * Copyright 2015 Rongall.com. All Rights Reserved.
 * =====================================================================================
 * Filename:  word2vec_vob.cc
 * Description:   description
 * Author: Koth(Yaowen Chen)
 *
 */
#include <ctime>
#include <random>
#include <cmath>
#include "utils/basic_string_util.h"
#include "utils/word2vec_vob.h"
#include "base/base.h"
// #include "re2/re2.h"
namespace utils {


namespace {

static std::string map_word(const std::string& word) {
  return word;
}
}

void Word2vecVocab::SetMapword(bool mapword) {
  map_word_ = mapword;
}
bool Word2vecVocab::GetMapword() {
  return map_word_;
}

std::vector<float> Word2vecVocab::GetFeatureOrEmpty(const std::string& word) {
  if (f_map_.find(word) != f_map_.end()) {
    std::vector<float> ret(f_map_[word].vect.begin(), f_map_[word].vect.end());
    return ret;
  } else {
    return std::vector<float>();
  }
}
bool Word2vecVocab::DumpBasicVocab(const std::string& path) {
  FILE *fp = fopen(path.c_str(), "w");
  if (fp == NULL) {
    fprintf(stderr, "open file for write error:%s\n", path.c_str());
    return false;
  }
  for (auto it = f_map_.begin(); it != f_map_.end(); ++it) {
    fprintf(fp, "%s\t%d\n", it->first.c_str(), it->second.idx);
  }
  fclose(fp);
  return true;
}
bool Word2vecVocab::Load(const std::string& path) {
  FILE *fp = fopen(path.c_str(), "r");
  if (fp == NULL) {
    fprintf(stderr, "open file error:%s\n", path.c_str());
    return false;
  }
  char line[4096] = {0};
  bool first = true;
  char *ptr = NULL;
  int tn = 0;
  float *  x2 = NULL;
  std::string secondToken;
  while (fgets(line, sizeof(line) - 1, fp)) {
    int nn = strlen(line);
    while (nn && (line[nn - 1] == '\n' || line[nn - 1] == '\r')) {
      nn -= 1;
    }
    if (nn <= 0) {
      continue;
    }
    std::vector<std::string> terms;
    BasicStringUtil::SplitString(line, nn, ' ', &terms);
    if (first) {
      first = false;
      f_dim_ = atoi(terms[1].c_str());
      avg_vals_ = new float[f_dim_];
      std_vals_ = new float[f_dim_];
      x2 = new float[f_dim_];
      memset(avg_vals_, 0, sizeof(float)*f_dim_);
      memset(std_vals_, 0, sizeof(float)*f_dim_);
      memset(x2, 0, sizeof(float)*f_dim_);
      fprintf(stderr, "got dim:%d\n", f_dim_);
      continue;
    }
    nn = terms.size();
    if (nn != (f_dim_ + 1)) {
      fprintf(stderr, "line len not comformed to dimension:%s\n", line);
      if (x2) delete[] x2;
      return false;
    }
    const std::string& word = terms[0];

    if (f_map_.find(word) != f_map_.end()) {
      fprintf(stderr, "duplicate word:%s\n", word.c_str());
      if (x2) delete[] x2;
      return false;
    }
    f_map_.insert(std::make_pair(word, WV()));
    std::vector<float>& features = f_map_[word].vect;
    f_map_[word].idx = f_map_.size() - 1;
    if (static_cast<int>(f_map_.size()) == 1) {
      CHECK(word == "</s>") << "first tok should be </s>";
    } else if (static_cast<int>(f_map_.size()) == 2) {
      secondToken = word;
    }
    if (word == "<UNK>") {
      f_map_[word].idx = 1;
      f_map_[secondToken].idx = f_map_.size() - 1;
      secondToken = word;
    }
    for (int i = 1; i < nn; i++) {
      float fv = strtof(terms[i].c_str(), &ptr);
      avg_vals_[i - 1] += fv;
      x2[i - 1] += fv * fv;
      features.push_back(fv);
    }
    tn += 1;
  }
  if (tn > 0) {
    VLOG(0) << "got tn:" << tn << ",max id=" << (f_map_.size() - 1);
    for (int i = 0; i < f_dim_; i++) {
      avg_vals_[i] /= tn;
      x2[i] = x2[i] / tn;
      std_vals_[i] = sqrt(x2[i] - avg_vals_[i] * avg_vals_[i]);
      VLOG(0) << " f[" << i << "] avg:" << avg_vals_[i] << ",std:" << std_vals_[i];
    }
  }
  CHECK(secondToken == "<UNK>") << "second token should be '<UNK>' ";
  if (x2)delete[] x2;
  fclose(fp);
  std::srand(std::time(0));
  return true;
}
int Word2vecVocab::GetWordIndex(const std::string& word) {
  if (f_map_.find(word) != f_map_.end()) {
    return f_map_[word].idx;
  } else if (map_word_) {
    std::string mword = map_word(word);
    auto it = f_map_.find(mword);
    if (it != f_map_.end()) {
      return it->second.idx;
    } else {
      VLOG(0) << "not found map word:" << mword;
      // return </s>
      return 1;
    }
  } else {
    return 1;
  }
}
bool Word2vecVocab::GetVector(const std::string& word, std::vector<float>** vec, OOV_OPT opt) {
  if (f_map_.find(word) != f_map_.end()) {
    *vec = &f_map_[word].vect;
    return true;
  } else {
    if (opt == USE_BLANK) return false;
    if (opt == USE_OOV) {
      *vec = &f_map_[std::string("</s>")].vect;
    } else if (opt == USE_RANDOM) {
      oov_feature_.clear();
      std::random_device rd;
      std::mt19937 gen(rd());
      for (int i = 0; i < f_dim_; i++) {
        std::normal_distribution<> d(avg_vals_[i], std_vals_[i]);
        oov_feature_.push_back(static_cast<float>(d(gen)));
      }
      *vec = &oov_feature_;
    } else {
      if (oov_feature_.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        for (int i = 0; i < f_dim_; i++) {
          std::normal_distribution<> d(avg_vals_[i], std_vals_[i]);
          oov_feature_.push_back(static_cast<float>(d(gen)));
        }
      }
      *vec = &oov_feature_;
    }
    return false;
  }
}

int Word2vecVocab::GetTotalWord() {
  return f_map_.size();
}



}  //  namespace utils
