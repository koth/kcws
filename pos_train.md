### 词性标注训练过程


- 1)准备单词word2vec训练样本


``` 
    python kcws/train/prepare_pos.py  /e/data/people_2014  pos_lines.txt
```



- 2)使用word2vec导出即将使用的词词典

``` 
    bazel build -c opt third_party/word2vec:word2vec
	bazel-bin/third_party/word2vec/word2vec -train pos_lines.txt -min-count 5 -save-vocab pre_word_vec.txt
```
- 3)替换单词中的UNK


``` 
 python kcws/train/replace_unk.py  pre_word_vec.txt pos_lines.txt pos_lines_with_unk.txt
```

- 4)训练词向量

``` 
bazel-bin/third_party/word2vec/word2vec  -train pos_lines_with_unk.txt -output word_vec.txt -size 150 -window 5 -sample 1e-4 -negative 5 -hs 0 -binary 0  -cbow 0 -iter 3 -min-count 5 -hs 1
```

- 5)统计词性tag出现频次，生成词性tag集合
  
``` 
python kcws/train/stats_pos.py  /e/data/people_2014 pos_vocab.txt  lines_withpos.txt
```

- 6)生成训练样本
  
``` 
 bazel build -c opt kcws/train:generate_pos_train
```


``` 
 bazel-bin/kcws/train/generate_pos_train word_vec.txt char_vec.txt  pos_vocab.txt  /e/data/people_2014  pos_train.txt
```

以上char_vec.txt可使用分词中相同的文件


 
- 7)去重，乱序，分开训练集，测试集

   

``` 
sort -u pos_train.txt>pos_train.u
shuf pos_train.u >pos_train.txt
head -n 230000 pos_train.txt >train.txt
tail -n 51362 pos_train.txt >test.txt
``` 

- 8)训练

``` 
python kcws/train/train_pos.py --train_data_path train.txt --test_data_path test.txt --log_dir pos_logs --word_word2vec_path word_vec.txt --char_word2vec_path char_vec.txt 
```


- 9)模型导出

```
python tools/freeze_graph.py --input_graph pos_logs/graph.pbtxt --input_checkpoint pos_logs/model.ckpt --output_node_names "transitions,Reshape_9" --output_graph kcws/models/pos_model.pbtxt
```
