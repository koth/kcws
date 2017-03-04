/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  sentence_breaker.h
 * Author:  Koth
 * Create Time: 2016-11-23 21:54:41
 * Description:
 *
 */
#ifndef KCWS_SENTENCE_BREAKER_H_
#define KCWS_SENTENCE_BREAKER_H_
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "utils/basic_string_util.h"
namespace kcws {

class SentenceBreaker {
 public:
  explicit SentenceBreaker(int maxLen);
  virtual ~SentenceBreaker();
  bool breakSentences(const UnicodeStr& text,
                      std::vector<UnicodeStr>* lines);

 private:
  static char*  kInlineMarks[];
  static char* kBreakMarks[];

  bool is_inline_mark(UnicodeCharT uch) ;
  bool is_break_mark(UnicodeCharT uch) ;

  std::unordered_map<UnicodeCharT, UnicodeCharT> inline_marks_;
  std::unordered_set<UnicodeCharT> break_marks_;
  std::unordered_set<UnicodeCharT> inline_marks_set_;
  int max_len_;
};
}  // namespace kcws

#endif  // KCWS_SENTENCE_BREAKER_H_
