/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  py_word2vec_vob.cc
 * Author:  Koth Chen
 * Create Time: 2016-07-25 18:46:27
 * Description:
 *
 */
#include "third_party/pybind11/pybind11.h"
#include "third_party/pybind11/stl.h"
#include "word2vec_vob.h"
namespace py = pybind11;

PYBIND11_PLUGIN(w2v) {
  py::module m("w2v", "python binding for  word2vec vocab");
  py::class_<utils::Word2vecVocab>(m, "Word2vecVocab", "python class Word2vecVocab")
  .def(py::init())
  .def("Load", &utils::Word2vecVocab::Load, "load word2vec from text file")
  .def("SetMapword", &utils::Word2vecVocab::SetMapword, "set whether map to word")
  .def("GetFeature", &utils::Word2vecVocab::GetFeatureOrEmpty, "get word embedding or empty if not exist")
  .def("GetTotalWord", &utils::Word2vecVocab::GetTotalWord, "get total words")
  .def("GetWordIndex", &utils::Word2vecVocab::GetWordIndex, "get word idx")
  .def("DumpBasicVocab", &utils::Word2vecVocab::DumpBasicVocab, "dump the word2vec vocab into basic mode");
  return m.ptr();
}