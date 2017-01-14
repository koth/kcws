/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  test_ac_scanner.cc
 * Author:  Koth
 * Create Time: 2016-12-09 17:02:56
 * Description:
 *
 */
#include <cstring>

#include "base/base.h"
#include "kcws/cc/ac_scanner.h"
#include "utils/basic_string_util.h"

DEFINE_string(test_string, "挑战中共创辉煌国际", "the test string");
class TestScanReporter: public ScanReporter<uint32_t> {
 public:
  bool callback(uint32_t pos, uint32_t& data, size_t len) override {
    VLOG(0) << "got data:" << data << ",at pos:" << pos << ",len:" << len;
    return false;
  }
};
int main(int argc,  char* argv[]) {
  FLAGS_v = 0;
  FLAGS_logtostderr = true;
  base::Init(argc, argv);
  AcScanner<UnicodeStr, uint32_t> ac_scanner;
  const char* dicts[] = {
    "中共",
    "共创",
    "挑战",
    "辉煌",
    "辉煌国际"
  };
  for (size_t i = 0; i < sizeof(dicts) / sizeof(char*); i++) {
    UnicodeStr ustr;
    BasicStringUtil::u8tou16(dicts[i], strlen(dicts[i]), ustr);
    ac_scanner.pushNode(ustr, i);
  }
  ac_scanner.buildFailNode();
  VLOG(0) << "total node:" << ac_scanner.NumItem();
  UnicodeStr testu;
  TestScanReporter reporter;
  BasicStringUtil::u8tou16(FLAGS_test_string.c_str(), FLAGS_test_string.size(), testu);
  VLOG(0) << "test string len:" << testu.size();
  bool ret = ac_scanner.doScan(testu, &reporter);
  VLOG(0) << "scan return:" << ret;
  return 0;
}