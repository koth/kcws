/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  viterbi_decode.h
 * Author:  Koth
 * Create Time: 2017-02-01 13:43:51
 * Description:
 *
 */
#ifndef KCWS_CC_VITERBI_DECODE_H_
#define KCWS_CC_VITERBI_DECODE_H_
#include <vector>
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
namespace kcws {
void get_best_path(
  const Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>, Eigen::Aligned>& predictions,
  int sentenceIdx,
  int nn,
  const std::vector<std::vector<float>>& trans,
  int** bp,
  float** scores,
  std::vector<int>& resultTags,
  int ntags);

int viterbi_decode(
  const Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>, Eigen::Aligned>& predictions,
  int sentenceIdx,
  int nn,
  const std::vector<std::vector<float>>& trans,
  int** bp,
  float** scores,
  int ntags);

}  // namespace kcws
#endif  // KCWS_CC_VITERBI_DECODE_H_
