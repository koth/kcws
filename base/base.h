// Copyright Koth 2016

#ifndef BASE_BASE_H_
#define BASE_BASE_H_

#include <stdint.h>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "third_party/gflags/include/gflags/gflags.h"
#include "third_party/glog/include/glog/logging.h"




namespace base {

void Init(int argc, char** argv);

}  // namesapace base

#endif  // BASE_BASE_H_
