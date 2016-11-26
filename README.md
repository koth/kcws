
### 背景


[97.5%准确率的深度学习中文分词（字嵌入+Bi-LSTM+CRF）] (https://mp.weixin.qq.com/s?__biz=MjM5ODIzNDQ3Mw==&mid=2649966433&idx=1&sn=be6c0e5485003d6f33804261df7c3ecf&chksm=beca376789bdbe71ef28c509776132d96e7e662be0adf0460cfd9963ad782b32d2d5787ff499&mpshare=1&scene=2&srcid=1122cZnCbEKZCCzf9LOSAyZ6&from=timeline&key=&ascene=2&uin=&devicetype=android-19&version=26031f30&nettype=WIFI)


### 构建

1. 安装好bazel代码构建工具，clone下来tensorflow项目代码，配置好(./configure)
2. clone 本项目地址到tensorflow同级目录，切换到本项目代码目录，运行./configure
3. 编译后台服务 

   > bazel build //kcws/cc:seg_backend_api


### 训练

1. 关注待字闺中公众号 回复 kcws 获取语料下载地址：
   
   ![logo](https://github.com/koth/kcws/blob/master/docs/qrcode_dzgz.jpg?raw=true "待字闺中")
   
   
2. 解压语料到一个目录

3. 切换到代码目录，运行:
  > python kcws/train/process_anno_file.py <语料目录> chars_for_w2v.txt
  
  > 使用word2vec 训练 chars_for_w2v (注意-binary 0),得到字嵌入结果vec.txt
  
  > bazel build kcws/train:generate_training 
  
  > ./bazel-bin/kcws/train/generate_training vec.txt <语料目录> all.txt
  
  > python kcws/train/filter_sentence.py all.txt  （得到train.txt , test.txt)

4. 安装好tensorflow,切换到kcws代码目录，运行:
  > python kcws/train/train_cws_lstm.py --word2vec_path vec.txt --train_data_path <绝对路径到train.txt> --test_data_path test.txt --max_sentence_len 80 --learning_rate 0.001
  
 
### demo
http://45.32.100.248:9090/

### 深度学习交流群

为了 控制群质量 大家先加我微信号 kothme
加群时备注自己从事的方向 最好提了2，3个深度学习术语
我拉大家 进群


