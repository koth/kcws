
#ifndef UTILS_VOCAB_H_
#define UTILS_VOCAB_H_
#include <string>
namespace utils {
class Vocab {
 public:
  virtual ~Vocab() {};
  virtual bool Load(const std::string& path) = 0;
  virtual int GetWordIndex(const std::string& word) = 0;
  virtual int GetTotalWord() = 0;
};
}  // namespace utils
#endif  // UTILS_VOCAB_H_
