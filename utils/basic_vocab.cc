
#include <ctime>
#include <random>
#include <cmath>
#include "base/base.h"
#include "basic_string_util.h"
#include "basic_vocab.h"
namespace utils {

namespace {
static std::string map_word(const std::string& word) {
  return word;
}
}  // namespace
bool BasicVocab::Load(const std::string& path) {
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
    if (w_map_.find(word) != w_map_.end()) {
      fprintf(stderr, "duplicate word:%s\n", word.c_str());
      return false;
    }
    int idx = atoi(terms[1].c_str());
    w_map_[word] = idx;
    tn += 1;
  }
  fclose(fp);
  return true;
}
int BasicVocab::GetWordIndex(const std::string& word) {
  auto it = w_map_.find(word);
  if ( it != w_map_.end()) {
    return it->second;
  } else {
    if (!use_map_)return 0;
    std::string mword = map_word(word);
    it = w_map_.find(mword);
    if (it != w_map_.end()) {
      return it->second;
    } else {
      VLOG(0) << "not found map word:" << mword;
      return 0;
    }
  }
}


int BasicVocab::GetTotalWord() {
  return w_map_.size();
}


}  //  namespace utils
