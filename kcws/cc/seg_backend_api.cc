/*
 * Copyright 2016- 2018 Koth. All Rights Reserved.
 * =====================================================================================
 * Filename:  seg_backend_api.cc
 * Author:  Koth
 * Create Time: 2016-11-20 20:43:26
 * Description:
 *
 */
#include <string>
#include <thread>
#include <memory>

#include "base/base.h"
#include "utils/jsonxx.h"
#include "utils/basic_string_util.h"
#include "kcws/cc/demo_html.h"
#include "kcws/cc/tf_seg_model.h"
#include "kcws/cc/pos_tagger.h"
#include "third_party/crow/include/crow.h"
#include "tensorflow/core/platform/init_main.h"

DEFINE_int32(port, 9090, "the  api serving binding port");
DEFINE_string(model_path, "kcws/models/seg_model.pbtxt", "the model path");
DEFINE_string(vocab_path, "kcws/models/basic_vocab.txt", "char vocab path");
DEFINE_string(pos_model_path, "kcws/models/pos_model.pbtxt", "the pos tagging model path");
DEFINE_string(word_vocab_path, "kcws/models/word_vocab.txt", "word vocab path");
DEFINE_string(pos_vocab_path, "kcws/models/pos_vocab.txt", "pos vocab path");
DEFINE_int32(max_sentence_len, 80, "max sentence len ");
DEFINE_string(user_dict_path, "", "user dict path");
DEFINE_int32(max_word_num, 50, "max num of word per sentence ");
class SegMiddleware {
 public:
  struct context {};
  SegMiddleware() {}
  ~SegMiddleware() {}
  void before_handle(crow::request& req, crow::response& res, context& ctx) {}
  void after_handle(crow::request& req, crow::response& res, context& ctx) {}
 private:
};
int main(int argc, char* argv[]) {
  tensorflow::port::InitMain(argv[0], &argc, &argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  crow::App<SegMiddleware> app;
  kcws::TfSegModel model;
  CHECK(model.LoadModel(FLAGS_model_path,
                        FLAGS_vocab_path,
                        FLAGS_max_sentence_len,
                        FLAGS_user_dict_path))
      << "Load model error";
  if (!FLAGS_pos_model_path.empty()) {
    kcws::PosTagger* tagger = new kcws::PosTagger;
    CHECK(tagger->LoadModel(FLAGS_pos_model_path,
                            FLAGS_word_vocab_path,
                            FLAGS_vocab_path,
                            FLAGS_pos_vocab_path,
                            FLAGS_max_word_num)) << "load pos model error";
    model.SetPosTagger(tagger);
  }
  CROW_ROUTE(app, "/tf_seg/api").methods("POST"_method)
  ([&model](const crow::request & req) {
    jsonxx::Object obj;
    int status = -1;
    std::string desc = "OK";
    std::string gotReqBody = req.body;
    VLOG(0) << "got body:";
    fprintf(stderr, "%s\n", gotReqBody.c_str());
    jsonxx::Object toRet;
    if (obj.parse(gotReqBody) && obj.has<std::string>("sentence")) {
      std::string sentence = obj.get<std::string>("sentence");
      std::vector<std::string> result;
      std::vector<std::string> tags;
      if (model.Segment(sentence, &result, &tags)) {
        status = 0;
        jsonxx::Array rarr;
        if (result.size() == tags.size()) {
          int nl = result.size();
          for (int i = 0; i < nl; i++) {
            jsonxx::Object obj;
            obj << "tok" << result[i];
            obj << "pos" << tags[i];
            rarr << obj;
          }
        } else {
          for (std::string str : result) {
            rarr << str;
          }
        }
        toRet << "segments" << rarr;
      }
    } else {
      desc = "Parse request error";
    }
    toRet << "status" << status;
    toRet << "msg" << desc;
    return crow::response(toRet.json());
  });
  CROW_ROUTE(app, "/")([](const crow::request & req) {
    return crow::response(std::string(reinterpret_cast<char*>(&kcws_cc_demo_html[0]), kcws_cc_demo_html_len));
  });
  app.port(FLAGS_port).multithreaded().run();
  return 0;
}
