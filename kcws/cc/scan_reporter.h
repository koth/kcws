/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  scan_reporter.h
 * Author:  Koth
 * Create Time: 2016-12-09 16:50:49
 * Description:
 *
 */
#ifndef KCWS_SCAN_REPORTER_H_
#define KCWS_SCAN_REPORTER_H_
#include <stdint.h>
#include <cstdlib>
template<typename T>
class ScanReporter {
 public:
  virtual bool callback(uint32_t pos, T& data, size_t len) = 0;
  virtual ~ScanReporter() {
  }
};


#endif  // KCWS_SCAN_REPORTER_H_ 
