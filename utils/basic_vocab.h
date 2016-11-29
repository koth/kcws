
#ifndef UTILS_BASIC_VOCAB_H_
#define UTILS_BASIC_VOCAB_H_
#include <vector>
#include <string>
#include <unordered_map>

#include "vocab.h"

namespace utils {
class BasicVocab: public Vocab {
 public:
  BasicVocab() {use_map_ = false;}
  BasicVocab(bool useMap): use_map_(useMap) {}
  bool Load(const std::string& path) override;
  int GetWordIndex(const std::string& word) override;
  int GetTotalWord() override;
 private:
  std::unordered_map<std::string, int> w_map_;
  bool use_map_;
};
}  // namespace utils
#endif  // UTILS_BASIC_VOCAB_H_
