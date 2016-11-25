

### 构建

1. 安装好bazel代码构建工具，clone下来tensorflow项目代码，配置好(./configure)
2. clone 本项目地址到tensorflow同级目录，切换到本项目代码目录，运行./configure
3. 编译后台服务 

   > bazel build //kcws/cc:seg_backend_api


### 训练

1. 关注待字闺中公众号 回复 kcws 获取语料下载地址：
    ![logo]( https://github.com/koth/kcws/blob/master/docs/qrcode_dzgz.jpg?raw=true "待字闺中")
   
2. 解压语料到一个目录

3. 切换到代码目录，运行:
  > pyton kcws/train/process_anno_file <语料目录> chars_for_w2v.txt
  
  > 使用word2vec 训练 chars_for_w2v (注意-binary 0),得到字嵌入结果vec.txt
  
  > bazel build kcws/train:generate_training 
  
  > ./bazel-bin/kcws/train/generate_training vec.txt <语料目录> all.txt
  
  > python kcws/train/filter_sentence.py all.txt  （得到train.txt , test.txt)

4. 安装好tensorflow,切换到kcws代码目录，运行:
  > python kcws/train/train_cws_lstm.py --word2vec_path vec.txt --train_data_path <绝对路径到train.txt> --test_data_path test.txt --max_sentence_len 80 --learning_rate 0.001
  
 
  

### 有问题欢迎反馈， 有兴趣请加入 微信 "深度学习交流群"：

