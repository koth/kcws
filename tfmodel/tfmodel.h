/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  tfmodel.h
 * Author:  Koth
 * Create Time: 2017-02-01 13:34:04
 * Description:
 *
 */
#ifndef TF_TFMODEL_H_
#define TF_TFMODEL_H_
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include "tensorflow/core/framework/types.pb.h"
#include "tensorflow/core/public/session.h"

namespace tf {
class TfModel {
 public:
  virtual ~TfModel();
  virtual bool Load(const std::string& path);
  bool Eval(const std::vector<std::pair<std::string, tensorflow::Tensor> >& inputTensors,
            const std::vector<std::string>& outputNames,
            std::vector<tensorflow::Tensor>& outputTensors);

 protected:
  std::unique_ptr<tensorflow::Session> session_;
};

}  // namespace tf
#endif  // TF_TFMODEL_H_


