# -*- coding: utf-8 -*-
# @Author: Koth Chen
# @Date:   2016-07-26 13:48:32
# @Last Modified by:   Koth
# @Last Modified time: 2017-04-07 23:04:45
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np

import tensorflow as tf
import os
from idcnn import Model as IdCNN
from bilstm import Model as BiLSTM
FLAGS = tf.app.flags.FLAGS

tf.app.flags.DEFINE_string('train_data_path', "newcorpus/2014_train.txt",
                           'Training data dir')
tf.app.flags.DEFINE_string('test_data_path', "newcorpus/2014_test.txt",
                           'Test data dir')
tf.app.flags.DEFINE_string('log_dir', "logs", 'The log  dir')
tf.app.flags.DEFINE_string("word2vec_path", "newcorpus/vec.txt",
                           "the word2vec data path")

tf.app.flags.DEFINE_integer("max_sentence_len", 80,
                            "max num of tokens per query")
tf.app.flags.DEFINE_integer("embedding_size", 50, "embedding size")
tf.app.flags.DEFINE_integer("num_tags", 4, "BMES")
tf.app.flags.DEFINE_integer("num_hidden", 100, "hidden unit number")
tf.app.flags.DEFINE_integer("batch_size", 100, "num example per mini batch")
tf.app.flags.DEFINE_integer("train_steps", 150000, "trainning steps")
tf.app.flags.DEFINE_float("learning_rate", 0.001, "learning rate")
tf.app.flags.DEFINE_bool("use_idcnn", True, "whether use the idcnn")
tf.app.flags.DEFINE_integer("track_history", 6, "track max history accuracy")


def do_load_data(path):
    x = []
    y = []
    fp = open(path, "r")
    for line in fp.readlines():
        line = line.rstrip()
        if not line:
            continue
        ss = line.split(" ")
        assert (len(ss) == (FLAGS.max_sentence_len * 2))
        lx = []
        ly = []
        for i in range(FLAGS.max_sentence_len):
            lx.append(int(ss[i]))
            ly.append(int(ss[i + FLAGS.max_sentence_len]))
        x.append(lx)
        y.append(ly)
    fp.close()
    return np.array(x), np.array(y)


class Model:
    def __init__(self, embeddingSize, distinctTagNum, c2vPath, numHidden):
        self.embeddingSize = embeddingSize
        self.distinctTagNum = distinctTagNum
        self.numHidden = numHidden
        self.c2v = self.load_w2v(c2vPath, FLAGS.embedding_size)
        self.words = tf.Variable(self.c2v, name="words")
        layers = [
            {
                'dilation': 1
            },
            {
                'dilation': 1
            },
            {
                'dilation': 2
            },
        ]
        if FLAGS.use_idcnn:
            self.model = IdCNN(layers, 3, FLAGS.num_hidden, FLAGS.embedding_size,
                               FLAGS.max_sentence_len, FLAGS.num_tags)
        else:
            self.model = BiLSTM(
                FLAGS.num_hidden, FLAGS.max_sentence_len, FLAGS.num_tags)
        self.trains_params = None
        self.inp = tf.placeholder(tf.int32,
                                  shape=[None, FLAGS.max_sentence_len],
                                  name="input_placeholder")
        pass

    def length(self, data):
        used = tf.sign(tf.abs(data))
        length = tf.reduce_sum(used, reduction_indices=1)
        length = tf.cast(length, tf.int32)
        return length

    def inference(self, X, reuse=None, trainMode=True):
        word_vectors = tf.nn.embedding_lookup(self.words, X)
        length = self.length(X)
        reuse = False if trainMode else True
        if FLAGS.use_idcnn:
            word_vectors = tf.expand_dims(word_vectors, 1)
            unary_scores = self.model.inference(word_vectors, reuse=reuse)
        else:
            unary_scores = self.model.inference(
                word_vectors, length, reuse=reuse)
        return unary_scores, length

    def loss(self, X, Y):
        P, sequence_length = self.inference(X)
        log_likelihood, self.transition_params = tf.contrib.crf.crf_log_likelihood(
            P, Y, sequence_length)
        loss = tf.reduce_mean(-log_likelihood)
        return loss

    def load_w2v(self, path, expectDim):
        fp = open(path, "r")
        print("load data from:", path)
        line = fp.readline().strip()
        ss = line.split(" ")
        total = int(ss[0])
        dim = int(ss[1])
        assert (dim == expectDim)
        ws = []
        mv = [0 for i in range(dim)]
        second = -1
        for t in range(total):
            if ss[0] == '<UNK>':
                second = t
            line = fp.readline().strip()
            ss = line.split(" ")
            assert (len(ss) == (dim + 1))
            vals = []
            for i in range(1, dim + 1):
                fv = float(ss[i])
                mv[i - 1] += fv
                vals.append(fv)
            ws.append(vals)
        for i in range(dim):
            mv[i] = mv[i] / total
        assert (second != -1)
        # append one more token , maybe useless
        ws.append(mv)
        if second != 1:
            t = ws[1]
            ws[1] = ws[second]
            ws[second] = t
        fp.close()
        return np.asarray(ws, dtype=np.float32)

    def test_unary_score(self):
        P, sequence_length = self.inference(self.inp,
                                            reuse=True,
                                            trainMode=False)
        return P, sequence_length


def read_csv(batch_size, file_name):
    filename_queue = tf.train.string_input_producer([file_name])
    reader = tf.TextLineReader(skip_header_lines=0)
    key, value = reader.read(filename_queue)
    # decode_csv will convert a Tensor from type string (the text line) in
    # a tuple of tensor columns with the specified defaults, which also
    # sets the data type for each column
    decoded = tf.decode_csv(
        value,
        field_delim=' ',
        record_defaults=[[0] for i in range(FLAGS.max_sentence_len * 2)])

    # batch actually reads the file and loads "batch_size" rows in a single
    # tensor
    return tf.train.shuffle_batch(decoded,
                                  batch_size=batch_size,
                                  capacity=batch_size * 50,
                                  min_after_dequeue=batch_size)


def test_evaluate(sess, unary_score, test_sequence_length, transMatrix, inp,
                  tX, tY):
    totalEqual = 0
    batchSize = FLAGS.batch_size
    totalLen = tX.shape[0]
    numBatch = int((tX.shape[0] - 1) / batchSize) + 1
    correct_labels = 0
    total_labels = 0
    for i in range(numBatch):
        endOff = (i + 1) * batchSize
        if endOff > totalLen:
            endOff = totalLen
        y = tY[i * batchSize:endOff]
        feed_dict = {inp: tX[i * batchSize:endOff]}
        unary_score_val, test_sequence_length_val = sess.run(
            [unary_score, test_sequence_length], feed_dict)
        for tf_unary_scores_, y_, sequence_length_ in zip(
                unary_score_val, y, test_sequence_length_val):
            # print("seg len:%d" % (sequence_length_))
            tf_unary_scores_ = tf_unary_scores_[:sequence_length_]
            y_ = y_[:sequence_length_]
            viterbi_sequence, _ = tf.contrib.crf.viterbi_decode(
                tf_unary_scores_, transMatrix)
            # Evaluate word-level accuracy.
            correct_labels += np.sum(np.equal(viterbi_sequence, y_))
            total_labels += sequence_length_
    accuracy = 100.0 * correct_labels / float(total_labels)
    print("Accuracy: %.3f%%" % accuracy)
    return accuracy


def inputs(path):
    whole = read_csv(FLAGS.batch_size, path)
    features = tf.transpose(tf.stack(whole[0:FLAGS.max_sentence_len]))
    label = tf.transpose(tf.stack(whole[FLAGS.max_sentence_len:]))
    return features, label


def train(total_loss):
    return tf.train.AdamOptimizer(FLAGS.learning_rate).minimize(total_loss)


def main(unused_argv):
    curdir = os.path.dirname(os.path.realpath(__file__))
    trainDataPath = tf.app.flags.FLAGS.train_data_path
    if not trainDataPath.startswith("/"):
        trainDataPath = curdir + "/../../" + trainDataPath
    graph = tf.Graph()
    with graph.as_default():
        model = Model(FLAGS.embedding_size, FLAGS.num_tags,
                      FLAGS.word2vec_path, FLAGS.num_hidden)
        print("train data path:", trainDataPath)
        X, Y = inputs(trainDataPath)
        tX, tY = do_load_data(tf.app.flags.FLAGS.test_data_path)
        total_loss = model.loss(X, Y)
        train_op = train(total_loss)
        test_unary_score, test_sequence_length = model.test_unary_score()
        sv = tf.train.Supervisor(graph=graph, logdir=FLAGS.log_dir)
        with sv.managed_session(master='') as sess:
            # actual training loop
            training_steps = FLAGS.train_steps
            trackHist = 0
            bestAcc = 0
            tf.train.write_graph(sess.graph.as_graph_def(),
                                 FLAGS.log_dir, "graph.pb", as_text=False)
            for step in range(training_steps):
                if sv.should_stop():
                    break
                try:
                    _, trainsMatrix = sess.run(
                        [train_op, model.transition_params])
                    # for debugging and learning purposes, see how the loss
                    # gets decremented thru training steps
                    if (step + 1) % 100 == 0:
                        print("[%d] loss: [%r]" %
                              (step + 1, sess.run(total_loss)))
                    if (step + 1) % 1000 == 0 or step == 0:
                        acc = test_evaluate(sess, test_unary_score,
                                            test_sequence_length, trainsMatrix,
                                            model.inp, tX, tY)
                        if acc > bestAcc:
                            if step:
                                sv.saver.save(
                                    sess, FLAGS.log_dir + '/best_model')
                            bestAcc = acc
                            trackHist = 0
                        elif trackHist > FLAGS.track_history:
                            print(
                                "always not good enough in last %d histories, best accuracy:%.3f"
                                % (trackHist, bestAcc))
                            break
                        else:
                            trackHist += 1
                except KeyboardInterrupt, e:
                    sv.saver.save(sess,
                                  FLAGS.log_dir + '/model',
                                  global_step=(step + 1))
                    raise e
            sv.saver.save(sess, FLAGS.log_dir + '/finnal-model')


if __name__ == '__main__':
    tf.app.run()
