/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  ac_scanner.h
 * Author:  Koth
 * Create Time: 2016-12-09 16:20:03
 * Description:
 *
 */
#ifndef KCWS_AC_SCANNER_H_
#define KCWS_AC_SCANNER_H_
#include <stdint.h>
#include <string.h>

#include <unordered_map>
#include <vector>
#include <queue>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <utility>

#include "base/base.h"

using std::queue;
using std::unordered_map;

template <typename S, typename T>
class AcScanner {
 public:
  class ScanReporter {
   public:
    virtual bool callback(uint32_t pos, T &data, size_t len) = 0;
    virtual ~ScanReporter() {}
  };
  typedef std::pair<S, T> StrNode;
  typedef std::vector<StrNode> StrNodeList;
  AcScanner();
  void pushNode(const S &str, const T &data);
  void buildFailNode();
  /**
   * return true to indicate should not scan any more!!
   */
  bool doScan(const S &word, ScanReporter *reporter) const;
  virtual ~AcScanner();
  int NumItem() { return num_node_ - 1; }

 private:
  int num_node_;

  class TrieNode {
   public:
    TrieNode() : is_leaf_(false), fail_node_(0), len_(0) {}
    ~TrieNode() {
      for (auto _it = transimition_.begin(); _it != transimition_.end();
           ++_it) {
        TrieNode *node = _it->second;
        if (node) {
          delete node;
        }
      }
    }
    T data_;
    bool is_leaf_;
    unordered_map<uint16_t, TrieNode *> transimition_;
    TrieNode *fail_node_;
    uint8_t len_;
  };
  TrieNode *root_;
};

template <typename S, typename T>
AcScanner<S, T>::AcScanner() {
  root_ = new TrieNode();
  root_->fail_node_ = root_;
  num_node_ = 1;
}
template <typename S, typename T>
AcScanner<S, T>::~AcScanner() {
  if (root_) {
    delete root_;
    root_ = NULL;
  }
}

template <typename S, typename T>
void AcScanner<S, T>::pushNode(const S &wstr, const T &data) {
  TrieNode *cur = root_;
  TrieNode *prev = NULL;
  uint32_t nn = wstr.length();
  if (0 == nn) return;
  uint16_t wc = 0;
  for (size_t i = 0; i < nn; i++) {
    wc = static_cast<uint16_t>(wstr[i]);
    prev = cur;
    auto it = cur->transimition_.find(wc);
    if (it == cur->transimition_.end()) {
      TrieNode *newNode = new TrieNode;
      cur = newNode;
      prev->transimition_[wc] = cur;
      num_node_++;
    } else {
      cur = it->second;
    }
  }

  if (cur->is_leaf_) {
    // duplicated string;
    VLOG(0) << "duplicated string found!";
    return;
  } else {  // mark it as a leaf node
    cur->is_leaf_ = true;
    cur->data_ = data;
    cur->len_ = nn;
  }
}
template <typename S, typename T>
void AcScanner<S, T>::buildFailNode() {
  queue<TrieNode *> todos;
  for (auto it = root_->transimition_.begin(); it != root_->transimition_.end();
       it++) {
    TrieNode *pNode = it->second;
    pNode->fail_node_ = root_;
    todos.push(pNode);
  }
  while (!todos.empty()) {
    TrieNode *parent = todos.front();
    todos.pop();
    for (auto it = parent->transimition_.begin();
         it != parent->transimition_.end(); it++) {
      uint16_t wc = static_cast<uint16_t>(it->first);
      TrieNode *cur = it->second;
      TrieNode *parentFailNode = parent->fail_node_;
      auto it2 = parentFailNode->transimition_.find(wc);
      while (parentFailNode != root_ &&
             it2 == parentFailNode->transimition_.end()) {
        parentFailNode = parentFailNode->fail_node_;
        it2 = parentFailNode->transimition_.find(wc);
      }
      if (it2 == parentFailNode->transimition_.end()) {
        cur->fail_node_ = root_;
      } else {
        //        printf(" got fainode from parent,wc=%lc", wc);
        cur->fail_node_ = it->second;
      }
      todos.push(cur);  // already processed cur
    }
  }
}

template <typename S, typename T>
bool AcScanner<S, T>::doScan(const S &word, ScanReporter *reporter) const {
  int nn = word.length();
  if (nn == 0) {
    return true;
  }
  uint16_t wc = 0;
  TrieNode *cur = root_;
  TrieNode *parent = NULL;
  TrieNode *prevFound = NULL;
  uint32_t prevPos = 0;
  //  bool needBackOne = false;
  for (int i = 0; i < nn;) {
    wc = static_cast<uint16_t>(word[i]);
    parent = cur;
    auto it = cur->transimition_.find(wc);
    if (it != cur->transimition_.end()) {
      cur = it->second;
    } else {
      cur = NULL;
    }

    if (cur == NULL) {  // fail at here
      if (prevFound) {
        if (reporter->callback(prevPos, prevFound->data_, prevFound->len_)) {
          return true;
        }
        // 从上一次匹配开头的下一个位置开始
        i = prevPos - prevFound->len_ + 2;
        prevFound = NULL;
        cur = root_;
        continue;
      } else {
        cur = parent->fail_node_;
        // if (parent != root_) {
        //   --i;
        // }
      }
    }
    // 如果一直命中会匹配最长的词汇
    if (cur->is_leaf_) {
      prevFound = cur;
      prevPos = i;
    }
    ++i;
  }
  if (prevFound) {
    if (reporter->callback(prevPos, prevFound->data_, prevFound->len_)) {
      return true;
    }
  }
  return false;
}
#endif   // KCWS_AC_SCANNER_H_
