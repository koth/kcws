# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2017-01-24 16:13:14
# @Last Modified by:   Koth
# @Last Modified time: 2017-04-07 23:02:50

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np

import tensorflow as tf
import os

FLAGS = tf.app.flags.FLAGS

tf.app.flags.DEFINE_string('train_data_path', "pos/train.txt",
                           'Training data dir')
tf.app.flags.DEFINE_string('test_data_path', "pos/test.txt", 'Test data dir')
tf.app.flags.DEFINE_string('log_dir', "pos_logs", 'The log  dir')
tf.app.flags.DEFINE_string("word_word2vec_path", "pos/word_vec.txt",
                           "the word2vec data path")
tf.app.flags.DEFINE_string("char_word2vec_path", "pos/char_vec.txt",
                           "the charater word2vec data path")
tf.app.flags.DEFINE_integer("max_sentence_len", 50,
                            "max num of tokens per query")
tf.app.flags.DEFINE_integer("embedding_word_size", 150, "embedding size")
tf.app.flags.DEFINE_integer("embedding_char_size", 50, "second embedding size")
tf.app.flags.DEFINE_integer("num_tags", 74, "num pos tags")
tf.app.flags.DEFINE_integer("char_window_size", 2,
                            "the window size of char convolution")
tf.app.flags.DEFINE_integer("max_chars_per_word", 5,
                            "max number of characters per word ")
tf.app.flags.DEFINE_integer("num_hidden", 100, "hidden unit number")
tf.app.flags.DEFINE_integer("batch_size", 64, "num example per mini batch")
tf.app.flags.DEFINE_integer("train_steps", 50000, "trainning steps")
tf.app.flags.DEFINE_float("learning_rate", 0.001, "learning rate")


def do_load_data(path):
    wx = []
    cx = []
    y = []
    fp = open(path, "r")
    ln = 0
    for line in fp:
        line = line.rstrip()
        ln += 1
        if not line:
            continue
        ss = line.split(" ")
        if len(ss) != (FLAGS.max_sentence_len *
                       (2 + FLAGS.max_chars_per_word)):
            print("[line:%d]len ss:%d,origin len:%d\n%s" %
                  (ln, len(ss), len(line), line))
        assert (len(ss) == (FLAGS.max_sentence_len *
                            (2 + FLAGS.max_chars_per_word)))
        lwx = []
        lcx = []
        ly = []
        for i in range(FLAGS.max_sentence_len):
            lwx.append(int(ss[i]))
            for k in range(FLAGS.max_chars_per_word):
                lcx.append(int(ss[FLAGS.max_sentence_len + i *
                                  FLAGS.max_chars_per_word + k]))
            ly.append(int(ss[i + FLAGS.max_sentence_len * (
                FLAGS.max_chars_per_word + 1)]))
        wx.append(lwx)
        cx.append(lcx)
        y.append(ly)
    fp.close()
    return np.array(wx), np.array(cx), np.array(y)


class Model:
    def __init__(self, distinctTagNum, w2vPath, c2vPath, numHidden):
        self.distinctTagNum = distinctTagNum
        self.numHidden = numHidden
        self.w2v = self.load_w2v(w2vPath, FLAGS.embedding_word_size)
        self.c2v = self.load_w2v(c2vPath, FLAGS.embedding_char_size)
        self.words = tf.Variable(self.w2v, name="words")
        self.chars = tf.Variable(self.c2v, name="chars")
        with tf.variable_scope('Softmax') as scope:
            self.W = tf.get_variable(
                shape=[numHidden * 2, distinctTagNum],
                initializer=tf.truncated_normal_initializer(stddev=0.01),
                name="weights",
                regularizer=tf.contrib.layers.l2_regularizer(0.001))
            self.b = tf.Variable(tf.zeros([distinctTagNum], name="bias"))
        with tf.variable_scope('CNN_Layer') as scope:
            self.filter = tf.get_variable(
                "filters_1",
                shape=[2, FLAGS.embedding_char_size, 1,
                       FLAGS.embedding_char_size],
                regularizer=tf.contrib.layers.l2_regularizer(0.0001),
                initializer=tf.truncated_normal_initializer(stddev=0.01),
                dtype=tf.float32)
        self.trains_params = None
        self.inp_w = tf.placeholder(tf.int32,
                                    shape=[None, FLAGS.max_sentence_len],
                                    name="input_words")
        self.inp_c = tf.placeholder(
            tf.int32,
            shape=[None, FLAGS.max_sentence_len * FLAGS.max_chars_per_word],
            name="input_chars")
        pass

    def length(self, data):
        used = tf.sign(tf.abs(data))
        length = tf.reduce_sum(used, reduction_indices=1)
        length = tf.cast(length, tf.int32)
        return length

    def char_convolution(self, vecs):
        conv1 = tf.nn.conv2d(vecs,
                             self.filter, [1, 1, FLAGS.embedding_char_size, 1],
                             padding='VALID')
        conv1 = tf.nn.relu(conv1)
        pool1 = tf.nn.max_pool(
            conv1,
            ksize=[1, FLAGS.max_chars_per_word - FLAGS.char_window_size + 1, 1,
                   1],
            strides=[1, FLAGS.max_chars_per_word - FLAGS.char_window_size + 1,
                     1, 1],
            padding='SAME')
        pool1 = tf.squeeze(pool1, [1, 2])
        return pool1

    def inference(self, wX, cX, reuse=None, trainMode=True):
        word_vectors = tf.nn.embedding_lookup(self.words, wX)
        char_vectors = tf.nn.embedding_lookup(self.chars, cX)
        char_vectors = tf.reshape(char_vectors, [-1, FLAGS.max_sentence_len,
                                                 FLAGS.max_chars_per_word,
                                                 FLAGS.embedding_char_size])
        char_vectors = tf.transpose(char_vectors, perm=[1, 0, 2, 3])
        char_vectors = tf.expand_dims(char_vectors, -1)
        length = self.length(wX)
        length_64 = tf.cast(length, tf.int64)

        # do conv
        def do_char_conv(x): return self.char_convolution(x)
        char_vectors_x = tf.map_fn(do_char_conv, char_vectors)
        char_vectors_x = tf.transpose(char_vectors_x, perm=[1, 0, 2])
        word_vectors = tf.concat([word_vectors, char_vectors_x], axis=2)
        # if trainMode:
        #  word_vectors = tf.nn.dropout(word_vectors, 0.5)
        reuse = None if trainMode else True
        with tf.variable_scope("rnn_fwbw", reuse=reuse) as scope:
            forward_output, _ = tf.nn.dynamic_rnn(
                tf.contrib.rnn.LSTMCell(self.numHidden,
                                        reuse=reuse),
                word_vectors,
                dtype=tf.float32,
                sequence_length=length,
                scope="RNN_forward")
            backward_output_, _ = tf.nn.dynamic_rnn(
                tf.contrib.rnn.LSTMCell(self.numHidden,
                                        reuse=reuse),
                inputs=tf.reverse_sequence(word_vectors,
                                           length_64,
                                           seq_dim=1),
                dtype=tf.float32,
                sequence_length=length,
                scope="RNN_backword")

        backward_output = tf.reverse_sequence(backward_output_,
                                              length_64,
                                              seq_dim=1)

        output = tf.concat([forward_output, backward_output], 2)
        output = tf.reshape(output, [-1, self.numHidden * 2])
        if trainMode:
            output = tf.nn.dropout(output, 0.5)

        matricized_unary_scores = tf.matmul(output, self.W) + self.b
        # matricized_unary_scores = tf.nn.log_softmax(matricized_unary_scores)
        unary_scores = tf.reshape(
            matricized_unary_scores,
            [-1, FLAGS.max_sentence_len, self.distinctTagNum])

        return unary_scores, length

    def loss(self, wX, cX, Y):
        P, sequence_length = self.inference(wX, cX)
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
        P, sequence_length = self.inference(self.inp_w,
                                            self.inp_c,
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
    decoded = tf.decode_csv(value,
                            field_delim=' ',
                            record_defaults=[
                                [0]
                                for i in range(FLAGS.max_sentence_len * (
                                    FLAGS.max_chars_per_word + 2))
                            ])

    # batch actually reads the file and loads "batch_size" rows in a single
    # tensor
    return tf.train.shuffle_batch(decoded,
                                  batch_size=batch_size,
                                  capacity=batch_size * 40,
                                  min_after_dequeue=batch_size)


def test_evaluate(sess, unary_score, test_sequence_length, transMatrix, inp_w,
                  inp_c, twX, tcX, tY):
    totalEqual = 0
    batchSize = FLAGS.batch_size
    totalLen = twX.shape[0]
    numBatch = int((twX.shape[0] - 1) / batchSize) + 1
    correct_labels = 0
    total_labels = 0
    for i in range(numBatch):
        endOff = (i + 1) * batchSize
        if endOff > totalLen:
            endOff = totalLen
        y = tY[i * batchSize:endOff]
        feed_dict = {inp_w: twX[i * batchSize:endOff],
                     inp_c: tcX[i * batchSize:endOff]}
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


def inputs(path):
    whole = read_csv(FLAGS.batch_size, path)
    features = tf.transpose(tf.stack(whole[0:FLAGS.max_sentence_len]))
    char_features = tf.transpose(tf.stack(whole[FLAGS.max_sentence_len:(
        FLAGS.max_chars_per_word + 1) * FLAGS.max_sentence_len]))
    label = tf.transpose(tf.stack(whole[(FLAGS.max_chars_per_word + 1) *
                                        FLAGS.max_sentence_len:]))
    return features, char_features, label


def train(total_loss):
    return tf.train.AdamOptimizer(FLAGS.learning_rate).minimize(total_loss)


def main(unused_argv):
    curdir = os.path.dirname(os.path.realpath(__file__))
    trainDataPath = tf.app.flags.FLAGS.train_data_path
    if not trainDataPath.startswith("/"):
        trainDataPath = curdir + "/../../" + trainDataPath
    graph = tf.Graph()
    with graph.as_default():
        model = Model(FLAGS.num_tags, FLAGS.word_word2vec_path,
                      FLAGS.char_word2vec_path, FLAGS.num_hidden)
        print("train data path:", trainDataPath)
        wX, cX, Y = inputs(trainDataPath)
        twX, tcX, tY = do_load_data(tf.app.flags.FLAGS.test_data_path)
        total_loss = model.loss(wX, cX, Y)
        train_op = train(total_loss)
        test_unary_score, test_sequence_length = model.test_unary_score()
        sv = tf.train.Supervisor(graph=graph, logdir=FLAGS.log_dir)
        with sv.managed_session(master='') as sess:
            # actual training loop
            training_steps = FLAGS.train_steps
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
                    if (step + 1) % 1000 == 0:
                        test_evaluate(sess, test_unary_score,
                                      test_sequence_length, trainsMatrix,
                                      model.inp_w, model.inp_c, twX, tcX, tY)
                except KeyboardInterrupt, e:
                    sv.saver.save(sess,
                                  FLAGS.log_dir + '/model',
                                  global_step=(step + 1))
                    raise e
            sv.saver.save(sess, FLAGS.log_dir + '/finnal-model')


if __name__ == '__main__':
    tf.app.run()
