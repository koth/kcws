
### 引用 

 
本项目模型BiLSTM+CRF参考论文：http://www.aclweb.org/anthology/N16-1030 ,IDCNN+CRF参考论文：https://arxiv.org/abs/1702.02098


### 构建

1. 安装好bazel代码构建工具，安装好tensorflow（目前本项目需要tf 1.0.0alpha版本以上)
2. 切换到本项目代码目录，运行./configure
3. 编译后台服务 

   > bazel build //kcws/cc:seg_backend_api


### 训练

1. 关注待字闺中公众号 回复 kcws 获取语料下载地址：
   
   ![logo](https://github.com/koth/kcws/blob/master/docs/qrcode_dzgz.jpg?raw=true "待字闺中")
   
   
2. 解压语料到一个目录

3. 切换到代码目录，运行:
  > python kcws/train/process_anno_file.py <语料目录> pre_chars_for_w2v.txt
  
  > bazel build third_party/word2vec:word2vec
  
  > 先得到初步词表
  
  > ./bazel-bin/third_party/word2vec/word2vec -train pre_chars_for_w2v.txt -save-vocab pre_vocab.txt -min-count 3
  
  > 处理低频词
  
  > python kcws/train/replace_unk.py pre_vocab.txt pre_chars_for_w2v.txt chars_for_w2v.txt
  > 
  > 训练word2vec
  > 
  > ./bazel-bin/third_party/word2vec/word2vec -train chars_for_w2v.txt -output vec.txt -size 50 -sample 1e-4 -negative 5 -hs 1 -binary 0 -iter 5
  > 
  > 构建训练语料工具
  > 
  > bazel build kcws/train:generate_training
  > 
  > 生成语料
  > 
  > ./bazel-bin/kcws/train/generate_training vec.txt <语料目录> all.txt
  > 
  > 得到train.txt , test.txt文件
  > 
  > python kcws/train/filter_sentence.py all.txt
  
4. 安装好tensorflow,切换到kcws代码目录，运行:

  > python kcws/train/train_cws.py --word2vec_path vec.txt --train_data_path <绝对路径到train.txt> --test_data_path test.txt --max_sentence_len 80 --learning_rate 0.001
  （默认使用IDCNN模型，可设置参数”--use_idcnn False“来切换BiLSTM模型)
  
5. 生成vocab
  > bazel  build kcws/cc:dump_vocab
  
  > ./bazel-bin/kcws/cc/dump_vocab vec.txt kcws/models/basic_vocab.txt
  
6. 导出训练好的模型
 >  python tools/freeze_graph.py --input_graph logs/graph.pbtxt  --input_checkpoint logs/model.ckpt --output_node_names  "transitions,Reshape_7"   --output_graph kcws/models/seg_model.pbtxt

7. 词性标注模型下载  (临时方案，后续文档给出词性标注模型训练，导出等）

   >  从 https://pan.baidu.com/s/1bYmABk 下载pos_model.pbtxt到kcws/models/目录下

8. 运行web service
 >  ./bazel-bin/kcws/cc/seg_backend_api --model_path=kcws/models/seg_model.pbtxt(绝对路径到seg_model.pbtxt>)   --vocab_path=kcws/models/basic_vocab.txt   --max_sentence_len=80

### 词性标注的训练说明：

https://github.com/koth/kcws/blob/master/pos_train.md

### 自定义词典
目前支持自定义词典是在解码阶段，参考具体使用方式请参考kcws/cc/test_seg.cc
字典为文本格式，每一行格式如下:
><自定义词条>\t<权重>

比如：
>蓝瘦香菇	4

权重为一个正整数，一般4以上，越大越重要
 
### demo
http://45.32.100.248:9090/

附： 使用相同模型训练的公司名识别demo:

http://45.32.100.248:18080




