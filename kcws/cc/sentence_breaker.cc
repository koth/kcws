/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  sentence_breaker.cc
 * Author:  Koth
 * Create Time: 2016-11-23 22:02:40
 * Description:
 *
 */
#include "sentence_breaker.h"  //NOLINT
#include "base/base.h"


namespace kcws {
char* SentenceBreaker::kInlineMarks[] = {
  "（", "）", "(", ")", "[", "]", "【", "】", "《", "》", "“", "”"
};
char* SentenceBreaker::kBreakMarks[] = {
  "。", ",", "，", " ", "\t", "?", "？", "!", "！", ";", "；"
};
SentenceBreaker::SentenceBreaker(int maxLen) {
  for (size_t i = 0; i < sizeof(kInlineMarks) / sizeof(char*); i += 2) {
    UnicodeStr ustr1;
    UnicodeStr ustr2;
    BasicStringUtil::u8tou16(kInlineMarks[i], strlen(kInlineMarks[i]), ustr1);
    BasicStringUtil::u8tou16(kInlineMarks[i + 1], strlen(kInlineMarks[i + 1]), ustr2);
    inline_marks_.insert(std::make_pair(ustr1[0], ustr2[0]));
    inline_marks_set_.insert(ustr1[0]);
    inline_marks_set_.insert(ustr2[0]);
  }
  for (size_t i = 0; i < sizeof(kBreakMarks) / sizeof(char*); i++) {
    UnicodeStr ustr;
    BasicStringUtil::u8tou16(kBreakMarks[i], strlen(kBreakMarks[i]), ustr);
    break_marks_.insert(ustr[0]);
  }
  max_len_ = maxLen;
}
bool SentenceBreaker::is_inline_mark(UnicodeCharT uch) {
  return inline_marks_.find(uch) != inline_marks_.end();
}
bool SentenceBreaker::is_break_mark(UnicodeCharT uch) {
  return break_marks_.find(uch) != break_marks_.end();
}
SentenceBreaker::~SentenceBreaker() = default;

bool SentenceBreaker::breakSentences(const UnicodeStr& text,
                                     std::vector<UnicodeStr>* lines) {
  UnicodeCharT markChar = 0;
  size_t nn = text.size();
  if (nn == 0) {
    return true;
  }
  size_t markPos = 0;
  for (size_t i = 0; i < nn; i++) {
    if (is_inline_mark(text[i])) {
      if (markChar == text[i]) {
        lines->push_back(text.substr(markPos, i - markPos + 1));
        markPos = i + 1;
        markChar = 0;
      } else {
        if (markPos != i) {
          lines->push_back(text.substr(markPos, i - markPos ));
          markPos = i;
        }
        markChar = inline_marks_[text[i]];
      }
    } else  if (markChar == 0) {
      if (is_break_mark(text[i]) ||
          (i - markPos + 1) >= static_cast<size_t>(max_len_)) {
        // Oops, too long
        lines->push_back(text.substr(markPos, i - markPos + 1));
        markPos = i + 1;
      }
    } else  if ((i - markPos + 1) >= static_cast<size_t>(max_len_) ) {
      // Oops, too long
      lines->push_back(text.substr(markPos, i - markPos + 1));
      markPos = i + 1;
      markChar = 0;
    }
  }
  if (markPos < nn) {
    lines->push_back(text.substr(markPos, nn - markPos));
  }
  return true;
}

}  // namespace kcws
