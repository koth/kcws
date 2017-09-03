/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  basic_string_util.h
 * Author:  Koth
 * Create Time: 2016-05-20 11:37:02
 * Description:
 *
 */
#ifndef UTILS_BASIC_STRING_UTIL_H_
#define UTILS_BASIC_STRING_UTIL_H_
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#define eq1(x, y) (tolower(x) == tolower(y))
#define eq2(x, y) ((x) == (y))
#define my_eq(t, x, y) ((t) ? eq2(x, y) : eq1(x, y))
typedef uint16_t UnicodeCharT;
typedef std::basic_string<UnicodeCharT> UnicodeStr;
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace std {
template <>
struct hash<UnicodeStr> {
  std::size_t operator()(const UnicodeStr& k) const {
    std::hash<std::u16string> hash_fn;
    std::u16string u16str(reinterpret_cast<const char16_t*>(k.c_str()),
                          k.size());
    // Use u16string for hash
    return hash_fn(u16str);
  }
};
}  // namespace std

class BasicStringUtil {
 public:
  static unsigned int LevenshteinDistance(const char* word1, const char* word2,
                                          bool caseSensitive = false) {
    unsigned int len1 = strlen(word1), len2 = strlen(word2);
    unsigned int* v =
        reinterpret_cast<unsigned int*>(calloc(len2 + 1, sizeof(unsigned int)));
    unsigned int i = 0, j = 0, current = 0, next = 0, cost = 0;

    /* strip common prefixes */
    while (len1 > 0 && len2 > 0 && my_eq(caseSensitive, word1[0], word2[0]))
      word1++, word2++, len1--, len2--;

    /* handle degenerate cases */
    if (!len1) return len2;
    if (!len2) return len1;

    /* initialize the column vector */
    for (j = 0; j < len2 + 1; j++) v[j] = j;

    for (i = 0; i < len1; i++) {
      /* set the value of the first row */
      current = i + 1;
      /* for each row in the column, compute the cost */
      for (j = 0; j < len2; j++) {
        /*
         * cost of replacement is 0 if the two chars are the same, or have
         * been transposed with the chars immediately before. otherwise 1.
         */
        cost = !(my_eq(caseSensitive, word1[i], word2[j]) ||
                 (i && j && my_eq(caseSensitive, word1[i - 1], word2[j]) &&
                  my_eq(caseSensitive, word1[i], word2[j - 1])));
        /* find the least cost of insertion, deletion, or replacement */
        next = std::min(std::min(v[j + 1] + 1, current + 1), v[j] + cost);
        /* stash the previous row's cost in the column vector */
        v[j] = current;
        /* make the cost of the next transition current */
        current = next;
      }
      /* keep the final cost at the bottom of the column */
      v[len2] = next;
    }
    free(v);
    return next;
  }
  static std::string TrimString(const std::string& s) {
    size_t nn = s.size();
    size_t i = 0;
    std::string ret(s);
    for (; i < nn; i++) {
      if ((i + 1) < nn && static_cast<unsigned char>(s[i]) == 0xC2 &&
          static_cast<unsigned char>(s[i + 1]) == 0xA0) {
        i += 1;
      } else if (s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n') {
        break;
      }
    }
    if (i) {
      ret = s.substr(i);
    }
    nn = ret.size();
    for (i = nn; i > 0; i--) {
      if (i > 1 && static_cast<unsigned char>(ret[i - 2]) == 0xC2 &&
          static_cast<unsigned char>(ret[i - 1]) == 0xA0) {
        i -= 1;
      } else if (ret[i - 1] != ' ' && ret[i - 1] != '\t' &&
                 ret[i - 1] != '\r' && ret[i - 1] != '\n') {
        break;
      }
    }
    if (i != nn) {
      ret = ret.substr(0, i);
    }
    return ret;
  }
  static void HexPrint(const char* buf, const size_t len) {
    size_t i;
    if (len == 0) return;
    printf("==========Hex Dump[%d]=========\n", (int)len);
    size_t nn = ((len - 1) / 16) + 1;
    for (size_t j = 0; j < nn; j++) {
      for (i = (16 * j); (i < len) && (i < (16 * j + 16)); i++) {
        if (i < len) {
          printf("%02X", buf[i] & 0xFF);
        } else {
          printf("  ");
        }
        if (i % 2) {
          putchar(' ');
        }
      }
      putchar('\n');
    }
  }
  static int SplitAsColonBackward(
      const char* str, int len,
      std::vector<std::pair<std::string, std::string>>* pOut) {
    int i = len - 1;
    int start = i;
    std::string fname;
    std::string fval;
    bool gotVal = false;
    // 先从后往前搜索
    for (; i >= 0; i--) {
      if (str[i] == ':') {
        if (gotVal) {
          // 再正向去搜索第一个空格符
          int j = i + 1;
          // 先去除空格
          while (j < len && str[j] == ' ') {
            j++;
          }
          int starti = j;
          bool needContinue = false;
          while (j < len && str[j] != ' ') {
            if (str[j] == ':') {
              std::string newVal(str + starti, j - starti);

              fval = newVal + ":" + fval;
              // fprintf(stderr, "newVal=%s\n", fval.c_str());
              needContinue = true;
              start = i - 1;
              break;
            }
            j++;
          }
          if (needContinue) {
            i -= 1;
            continue;
          }

          std::string newVal(str + starti, j - starti);
          //定位到非空格去
          while (j < len && str[j] == ' ') {
            j++;
          }
          fname = std::string(str + j, start - j + 1);
          // fprintf(stderr, "start=%d,j=%d,fname=%s,fval=%s,newval=%s\n",
          // start, j, fname.c_str(), fval.c_str(), newVal.c_str());

          pOut->push_back(std::make_pair(fname, fval));
          fval = newVal;
          start = i - 1;
          // fprintf(stderr, "start.1=%d\n", start);
        } else {
          fval = std::string(str + i + 1, start - i);
          start = i - 1;
          // fprintf(stderr, "start.2=%d\n", start);
          gotVal = true;
        }
      }
    }
    if (!gotVal) {
      return 0;
    }
    if (start < 0) {
      return -1;
    }
    fname = std::string(str, start + 1);
    pOut->push_back(std::make_pair(fname, fval));
    return pOut->size();
  }
  static std::string StripStringASCIIWhole(const std::string& str) {
    size_t nn = str.size();
    while (nn > 0 && (str[nn - 1] == ' ' || str[nn - 1] == '\t' ||
                      str[nn - 1] == '\r' || str[nn - 1] == '\n')) {
      nn -= 1;
    }
    size_t off = 0;
    while (off < nn && (str[off] == ' ' || str[off] == '\t' ||
                        str[off] == '\r' || str[off] == '\n')) {
      off += 1;
    }
    bool seeWhitespace = false;
    std::string ret;
    for (size_t k = off; k < nn; k++) {
      if (str[k] == ' ' || str[k] == '\t' || str[k] == '\r' || str[k] == '\n') {
        if (!seeWhitespace) {
          seeWhitespace = true;
          ret.append(1, ' ');
        }
      } else {
        seeWhitespace = false;
        ret.append(1, str[k]);
      }
    }
    return ret;
  }
  static std::string StripStringASCIINoSpaceLeft(const std::string& str) {
    size_t nn = str.size();
    while (nn > 0 && (str[nn - 1] == ' ' || str[nn - 1] == '\t' ||
                      str[nn - 1] == '\r' || str[nn - 1] == '\n')) {
      nn -= 1;
    }
    size_t off = 0;
    while (off < nn && (str[off] == ' ' || str[off] == '\t' ||
                        str[off] == '\r' || str[off] == '\n')) {
      off += 1;
    }
    std::string ret;
    for (size_t k = off; k < nn; k++) {
      if (str[k] == ' ' || str[k] == '\t' || str[k] == '\r' || str[k] == '\n') {
      } else {
        ret.append(1, str[k]);
      }
    }
    return ret;
  }
  static std::string StripStringASCII(const std::string& str) {
    size_t nn = str.size();
    while (nn > 0 && (str[nn - 1] == ' ' || str[nn - 1] == '\t' ||
                      str[nn - 1] == '\r' || str[nn - 1] == '\n')) {
      nn -= 1;
    }
    size_t off = 0;
    while (off < nn && (str[off] == ' ' || str[off] == '\t' ||
                        str[off] == '\r' || str[off] == '\n')) {
      off += 1;
    }
    return std::string(str.c_str() + off, nn - off);
  }
  static std::string StripString(const std::string& str) {
    size_t nn = str.size();
    while (nn > 0) {
      if ((str[nn - 1] == ' ' || str[nn - 1] == '\t' || str[nn - 1] == '\r' ||
           str[nn - 1] == '\n')) {
        nn -= 1;
      } else if (nn > 1 && static_cast<unsigned char>(str[nn - 2]) == 0xc2 &&
                 static_cast<unsigned char>(str[nn - 1]) == 0xa0) {
        nn -= 2;
      } else {
        break;
      }
    }

    size_t off = 0;
    while (nn > 0 && off < nn) {
      if (str[off] == ' ' || str[off] == '\t' || str[off] == '\r' ||
          str[off] == '\n') {
        off += 1;
      } else if (off < (nn - 1) && static_cast<unsigned>(str[off]) == 0xc2 &&
                 static_cast<unsigned>(str[off + 1]) == 0xa0) {
        off += 2;
      } else {
        break;
      }
    }
    return std::string(str.c_str() + off, nn - off);
  }
  static time_t StringToTime(const char* strTime, size_t len) {
    if (NULL == strTime) {
      return 0;
    }
    tm tm_;
    int year, month, day;
    sscanf(strTime, "%d-%d-%d", &year, &month, &day);
    tm_.tm_year = year - 1900;
    tm_.tm_mon = month - 1;
    tm_.tm_mday = day;
    tm_.tm_hour = 0;
    tm_.tm_min = 0;
    tm_.tm_sec = 0;
    tm_.tm_isdst = 0;
    time_t t_ = mktime(&tm_);  //已经减了8个时区
    return t_;                 //秒时间
  }
  static bool TrimSpace(const std::string& src, std::string* dest) {
    size_t firstNonSpace = 0;
    size_t len = src.size();
    size_t lastNonSpace = len;

    while (firstNonSpace < len &&
           ((src[firstNonSpace] == ' ') || (src[firstNonSpace] == '\t') ||
            (src[firstNonSpace] == '\n') || (src[firstNonSpace] == '\r'))) {
      firstNonSpace += 1;
    }
    while (lastNonSpace > 0 &&
           ((src[lastNonSpace - 1] == ' ') || (src[lastNonSpace - 1] == '\t') ||
            (src[lastNonSpace - 1] == '\n') ||
            (src[lastNonSpace - 1] == '\r'))) {
      lastNonSpace -= 1;
    }
    if (firstNonSpace > lastNonSpace) {
      dest->clear();
      return true;
    }
    if (firstNonSpace != 0 || lastNonSpace != len) {
      dest->assign(src.c_str() + firstNonSpace, lastNonSpace - firstNonSpace);
      return true;
    }
    dest->assign(src);
    return false;
  }
  static bool u8tou16(const char* src, size_t len, UnicodeStr& dest) {
    if (src == NULL) return true;
    UnicodeCharT stackBuf[1024] = {0};
    UnicodeCharT* ptr = stackBuf;
    size_t out_len = len;
    UnicodeCharT* destBuf = NULL;
    if (out_len > 1024) {
      destBuf = new UnicodeCharT[out_len];
    } else {
      destBuf = stackBuf;
    }
    if (destBuf == NULL) return false;
    size_t j = 0;
    unsigned char ubuf[2] = {0};
    for (size_t i = 0; i < len && j < out_len;) {
      unsigned char ch = (src[i] & 0xFF);
      if (ch < (unsigned short)0x80) {
        destBuf[j++] = (ch & 0x7F);
        i += 1;
      } else if (ch < (unsigned short)0xC0) {
        // need skip this guy!!!0x10xxxxxx, invalid start of utf-8 sequence!!
        destBuf[j++] = 0x3f;  // switch it into '?'
        i += 1;
      } else if (ch < (unsigned short)0xE0 && i + 1 < len) {
        // 110xxxxx 10xxxxxx
        ubuf[1] = (((ch & 0x1C) >> 2) & 0x7);
        ubuf[0] = ((((ch & 0x3) << 6)) | ((src[i + 1]) & 0x3F)) & 0xFF;
        ptr = static_cast<UnicodeCharT*>(static_cast<void*>(&ubuf[0]));
        destBuf[j++] = *(ptr);
        i += 2;
      } else if (ch < (unsigned short)0xF0 && i + 2 < len) {
        // 1110xxxx 10xxxxxx 10xxxxxx
        ubuf[1] = ((((ch & 0x0F) << 4) | ((src[i + 1] & 0x3C) >> 2)) & 0xFF);
        ubuf[0] = ((((src[i + 1] & 0x3) << 6)) | ((src[i + 2]) & 0x3F)) & 0xFF;
        ptr = static_cast<UnicodeCharT*>(static_cast<void*>(&ubuf[0]));
        destBuf[j++] = *ptr;
        i += 3;
      } else if (ch < (unsigned short)0xF8) {
        destBuf[j++] = 0x3f;  // switch it into '?'
        i += 4;
      } else if (ch < (unsigned short)0xFC) {
        destBuf[j++] = 0x3f;  // switch it into '?'
        i += 5;
      } else if (ch < (unsigned short)0xFE) {
        destBuf[j++] = 0x3f;  // switch it into '?'
        i += 6;
      } else {  // 0xFF
        destBuf[j++] = 0x3f;
        i += 1;
      }
    }

    dest.assign(destBuf, j);
    if (destBuf != stackBuf) delete[] destBuf;
    return (j > 0);
  }

  static bool u16tou8(const UnicodeCharT* src, size_t len, std::string& dest) {
    if (src == NULL) return true;
    char stackBuf[1024] = {0};
    size_t out_len = len * 3;
    char* destBuf = NULL;
    if (out_len > 1024) {
      destBuf = new char[out_len];
    } else {
      destBuf = stackBuf;
    }
    if (destBuf == NULL) return false;
    size_t j = 0;
    for (size_t i = 0; i < len && j < out_len; i++) {
      unsigned short uch = src[i];
      if (uch < (unsigned short)0x7F) {
        destBuf[j++] = (uch & 0x007F);
      } else if (uch < (unsigned short)0x7FF) {
        // 110xxxxx 10xxxxxx
        destBuf[j++] = ((((uch & 0x03C0) >> 6) & 0xFF) | (0xC0)) & 0xFF;
        destBuf[j++] = ((uch & 0x3F) | (0x80)) & 0xFF;
      } else {
        // 1110xxxx 10xxxxxx 10xxxxxx
        destBuf[j++] = ((((uch & 0xF000) >> 12) & 0xFF) | (0xE0)) & 0xFF;
        destBuf[j++] = ((((uch & 0x0FC0) >> 6) & 0xFF) | (0x80)) & 0xFF;
        destBuf[j++] = ((uch & 0x3F) | (0x80)) & 0xFF;
      }
    }
    dest.assign(destBuf, j);
    if (destBuf != stackBuf) delete[] destBuf;
    return (j > 0);
  }

  static int SplitString(const char* str, size_t len, char sepChar,
                         std::vector<std::string>* pOut) {
    const char* ptr = str;
    if (ptr == NULL || len == 0) {
      return 0;
    }
    size_t start = 0;
    while (start < len && (str[start] == sepChar)) {
      start += 1;
    }
    ptr = str + start;
    len = len - start;
    while (len > 0 && ptr[len - 1] == sepChar) {
      len -= 1;
    }
    if (len <= 0) {
      return 0;
    }
    size_t ps = 0;
    int nret = 0;
    for (size_t i = 0; i < len; i++) {
      if (ptr[i] == sepChar) {
        if (ptr[i - 1] != sepChar) {
          std::string ts(ptr + ps, i - ps);
          pOut->push_back(ts);
          nret += 1;
        }
        ps = i + 1;
      }
    }
    if (ps < len) {
      pOut->push_back(std::string(ptr + ps, len - ps));
      nret += 1;
    }
    return nret;
  }

  static std::string ReadFileContent(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "r");
    if (fp == nullptr) {
      fprintf(stderr, "read file [%s] error!\n", path.c_str());
      return std::string();
    }
    char buffer[4096] = {0};
    int nn = 0;
    std::string ret;
    do {
      nn = fread(buffer, sizeof(char), sizeof(buffer) / sizeof(char), fp);
      if (nn > 0) {
        ret.append(buffer, nn);
      }
    } while (nn > 0);
    fclose(fp);
    return ret;
  }

  static inline int CharByteLen(unsigned char ch) {
    if (unlikely((ch & 0xFC) == 0xFC))
      return 6;
    else if (unlikely((ch & 0xF8) == 0xF8))
      return 5;
    else if (unlikely((ch & 0xF0) == 0xF0))
      return 4;
    else if ((ch & 0xE0) == 0xE0)
      return 3;
    else if (unlikely((ch & 0xC0) == 0xC0))
      return 2;
    else if (unlikely(ch == 0))
      return 1;
    return 1;
  }

};  // class

namespace utils {
template <typename T>
std::string NumberToString(T Number) {
  std::ostringstream ss;
  ss << Number;
  return ss.str();
}

}  // namespace utils
#endif  // UTILS_BASIC_STRING_UTIL_H_