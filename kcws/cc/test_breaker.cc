/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  test_breaker.cc
 * Author:  Koth
 * Create Time: 2016-11-24 19:40:33
 * Description:
 *
 */
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>

#include "base/base.h"
#include "utils/basic_string_util.h"

#include "sentence_breaker.h"  //NOLINT

DEFINE_string(test_str, "", "the test string");

int main(int argc, char *argv[]) {
  FLAGS_v = 0;
  FLAGS_logtostderr = 1;
  base::Init(argc, argv);
  kcws::SentenceBreaker breaker(80);
  CHECK(!FLAGS_test_str.empty()) << "test string should be set";
  UnicodeStr ustr;
  CHECK(BasicStringUtil::u8tou16(FLAGS_test_str.c_str(), FLAGS_test_str.size(), ustr));
  std::vector<UnicodeStr> results;
  CHECK(breaker.breakSentences(ustr, &results)) << "break error";
  VLOG(0) << "results is :";
  for (auto u : results) {
    std::string todo;
    CHECK(BasicStringUtil::u16tou8(u.c_str(), u.size(), todo));
    VLOG(0) << todo;
  }
  return 0;
}
