/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  test_seg.cc
 * Author:  Koth
 * Create Time: 2016-11-20 12:13:21
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


#include "tf_seg_model.h"  //NOLINT
#include "sentence_breaker.h"  // NOLINT
#include "tensorflow/core/platform/init_main.h"

DEFINE_string(test_sentence, "", "the test string");
DEFINE_string(test_file, "", "the test file");
DEFINE_string(model_path, "", "the model path");
DEFINE_string(vocab_path, "", "vocab path");
DEFINE_string(user_dict_path, "", "user dict path");
DEFINE_int32(max_setence_len, 80, "max sentence len");

const int BATCH_SIZE = 2000;
int load_test_file(const std::string& path,
                   std::vector<std::string>* pstrs) {
  FILE *fp = fopen(path.c_str(), "r");
  if (fp == NULL) {
    VLOG(0) << "open file error:" << path;
    return 0;
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
    pstrs->push_back(std::string(line, nn));
    tn += 1;
  }
  fclose(fp);
  return tn;
}
int main(int argc, char *argv[]) {
  tensorflow::port::InitMain(argv[0], &argc, &argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_vocab_path.empty()) {
    VLOG(0) << "basic bocab path is not set";
    return 1;
  }
  if (FLAGS_model_path.empty()) {
    VLOG(0) << " model path is not set";
    return 1;
  }
  kcws::TfSegModel sm;
  CHECK(sm.LoadModel(FLAGS_model_path,
                     FLAGS_vocab_path,
                     FLAGS_max_setence_len,
                     FLAGS_user_dict_path))
      << "Load model error";
  if (!FLAGS_test_sentence.empty()) {
    std::vector<std::string> results;
    CHECK(sm.Segment(FLAGS_test_sentence, &results)) << "segment error";
    VLOG(0) << "results is :";
    for (auto str : results) {
      VLOG(0) << str;
    }
  } else if (!FLAGS_test_file.empty()) {
    kcws::SentenceBreaker breaker(FLAGS_max_setence_len);
    std::vector<std::string> teststrs;
    int ns = load_test_file(FLAGS_test_file, &teststrs);
    std::string todo;
    for (int i = 0; i < ns; i++) {
      todo.append(teststrs[i]);
    }
    UnicodeStr utodo;
    BasicStringUtil::u8tou16(todo.c_str(), todo.size(), utodo);
    std::vector<UnicodeStr> sentences;
    breaker.breakSentences(utodo, &sentences);

    VLOG(0) << "loaded :" << FLAGS_test_file << " ,got " << ns << " lines,"
            << sentences.size() << " sentences, " << utodo.size() << " characters";
    int batch = (sentences.size() - 1) / BATCH_SIZE + 1;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < batch; i++) {
      // VLOG(0) << "seg batch:" << i;
      int end = BATCH_SIZE * (i + 1);
      if (end > static_cast<int>(sentences.size())) {
        end = sentences.size();
      }
      std::vector<std::vector<kcws::SegTok>> results;
      std::vector<UnicodeStr>  todoSentences(sentences.begin() + (BATCH_SIZE * i), sentences.begin() + end);
      CHECK(sm.Segment(todoSentences, &results)) << "segment error";
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::steady_clock::now() - start);
    VLOG(0) << "spend " << duration.count() << " milliseconds for file:" << FLAGS_test_file;
  } else {
    VLOG(0) << "either test sentence or test file  should be set";
    return 1;
  }

  return 0;
}
