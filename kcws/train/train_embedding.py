# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-11-30 21:07:24
# @Last Modified by:   Koth
# @Last Modified time: 2016-12-01 13:04:36

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf
from tensorflow.contrib import learn
import os

FLAGS = tf.app.flags.FLAGS

tf.app.flags.DEFINE_string(
    'train_data_path', "/Users/tech/code/kcws/train.txt", 'Training data dir')
tf.app.flags.DEFINE_string('test_data_path', "./test.txt", 'Test data dir')
tf.app.flags.DEFINE_string('log_dir', "logs", 'The log  dir')
tf.app.flags.DEFINE_string('embedding_result', "embedding.txt", 'The log  dir')
tf.app.flags.DEFINE_integer("max_sentence_len", 5,
                            "max num of tokens per query")
tf.app.flags.DEFINE_integer("embedding_size", 25, "embedding size")
tf.app.flags.DEFINE_integer("num_hidden", 20, "hidden unit number")
tf.app.flags.DEFINE_integer("batch_size", 100, "num example per mini batch")
tf.app.flags.DEFINE_integer("train_steps", 50000, "trainning steps")
tf.app.flags.DEFINE_float("learning_rate", 0.001, "learning rate")
tf.app.flags.DEFINE_integer("num_words", 5902, "embedding size")


def do_load_data(path):
  x = []
  y = []
  fp = open(path, "r")
  for line in fp.readlines():
    line = line.rstrip()
    if not line:
      continue
    ss = line.split(" ")
    assert (len(ss) == (FLAGS.max_sentence_len + 1))
    lx = []
    for i in range(FLAGS.max_sentence_len):
      lx.append(int(ss[i]))
    x.append(lx)
    y.append(int(ss[FLAGS.max_sentence_len]))
  fp.close()
  return np.array(x), np.array(y)


class Model:
  def __init__(self, embeddingSize, numHidden):
    self.embeddingSize = embeddingSize
    self.numHidden = numHidden
    self.words = tf.Variable(
        tf.truncated_normal([FLAGS.num_words, embeddingSize]),
        name="words")
    with tf.variable_scope('Softmax') as scope:
      self.W = tf.get_variable(
          shape=[numHidden * 2, 2],
          initializer=tf.truncated_normal_initializer(stddev=0.01),
          name="weights",
          regularizer=tf.contrib.layers.l2_regularizer(0.001))
      self.b = tf.Variable(tf.zeros([2], name="bias"))
    self.inp = tf.placeholder(tf.int32,
                              shape=[None, FLAGS.max_sentence_len],
                              name="input_placeholder")
    self.tp = tf.placeholder(tf.int64, shape=[None], name="target_placeholder")
    pass

  def length(self, data):
    used = tf.sign(tf.abs(data))
    length = tf.reduce_sum(used, reduction_indices=1)
    length = tf.cast(length, tf.int32)
    return length

  def inference(self, X, reuse=None, trainMode=True):
    length = self.length(X)
    length_64 = tf.cast(length, tf.int64)
    word_vectors = tf.nn.embedding_lookup(self.words, X)
    if trainMode:
      word_vectors = tf.nn.dropout(word_vectors, 0.5)
    with tf.variable_scope("rnn_fwbw", reuse=reuse) as scope:
      _, forward_output = tf.nn.dynamic_rnn(
          tf.nn.rnn_cell.LSTMCell(self.numHidden),
          word_vectors,
          dtype=tf.float32,
          sequence_length=length,
          scope="RNN_forward")
      _, backward_output = tf.nn.dynamic_rnn(
          tf.nn.rnn_cell.LSTMCell(self.numHidden),
          inputs=tf.reverse_sequence(word_vectors,
                                     length_64,
                                     seq_dim=1),
          dtype=tf.float32,
          sequence_length=length,
          scope="RNN_backword")
    output = tf.concat(1, [forward_output[1], backward_output[1]])
    logit = tf.batch_matmul(output, self.W)
    return logit

  def loss(self, X, Y):
    P = self.inference(X)
    entroyp = tf.nn.sigmoid_cross_entropy_with_logits(P, Y)
    loss = tf.reduce_mean(entroyp)
    return loss

  def test_correct_num(self):
    logits = self.inference(self.inp, reuse=True, trainMode=False)
    targets = tf.argmax(logits, axis=1)
    return tf.reduce_sum(tf.cast(tf.equal(targets, self.tp), tf.int64))


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
      record_defaults=[[0] for i in range(FLAGS.max_sentence_len + 1)])

  # batch actually reads the file and loads "batch_size" rows in a single tensor
  return tf.train.shuffle_batch(decoded,
                                batch_size=batch_size,
                                capacity=batch_size * 50,
                                min_after_dequeue=batch_size)


def test_evaluate(sess, calcCorrectOp, inp, tp, tX, tY):
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
    feed_dict = {inp: tX[i * batchSize:endOff], tp: tY[i * batchSize:endOff]}
    count = sess.run([calcCorrectOp], feed_dict)
    correct_labels += count[0]
  accuracy = 100.0 * correct_labels / float(totalLen)
  print("Accuracy: %.2f%%" % accuracy)


def inputs(path):
  whole = read_csv(FLAGS.batch_size, path)
  features = tf.transpose(tf.pack(whole[0:FLAGS.max_sentence_len]))
  label = tf.one_hot(
      tf.transpose(tf.pack(whole[FLAGS.max_sentence_len])),
      depth=2)
  return features, label


def train(total_loss):
  return tf.train.AdamOptimizer(FLAGS.learning_rate).minimize(total_loss)


def write_embedding_result(sess, word_op, fp):
  all_words = sess.run(word_op)
  nn = len(all_words)
  nc = len(all_words[0])
  assert (nc == FLAGS.embedding_size)
  fp.write("%d %d\n" % (nn, FLAGS.embedding_size))
  for i in range(nn):
    line = str(i)
    for j in range(FLAGS.embedding_size):
      line += " " + str(all_words[i][j])
    fp.write("%s\n" % (line))


def main(unused_argv):
  curdir = os.path.dirname(os.path.realpath(__file__))
  trainDataPath = tf.app.flags.FLAGS.train_data_path
  if not trainDataPath.startswith("/"):
    trainDataPath = curdir + "/" + trainDataPath
  embedding_out = open(FLAGS.embedding_result, "w")
  graph = tf.Graph()
  with graph.as_default():
    model = Model(FLAGS.embedding_size, FLAGS.num_hidden)
    print("train data path:", trainDataPath)
    X, Y = inputs(trainDataPath)
    tX, tY = do_load_data(tf.app.flags.FLAGS.test_data_path)
    total_loss = model.loss(X, Y)
    train_op = train(total_loss)
    calc_correct_op = model.test_correct_num()
    sv = tf.train.Supervisor(graph=graph, logdir=FLAGS.log_dir)
    with sv.managed_session(master='') as sess:
      # actual training loop
      training_steps = FLAGS.train_steps
      for step in range(training_steps):
        if sv.should_stop():
          break
        try:
          _ = sess.run([train_op])
          # for debugging and learning purposes, see how the loss gets decremented thru training steps
          if step % 100 == 0:
            print("[%d] loss: [%r]" % (step, sess.run(total_loss)))
          if step % 1000 == 0:
            test_evaluate(sess, calc_correct_op, model.inp, model.tp, tX, tY)
        except KeyboardInterrupt, e:
          write_embedding_result(sess, model.words, embedding_out)
          sv.saver.save(sess, FLAGS.log_dir + '/model', global_step=step + 1)
          raise e
      write_embedding_result(sess, model.words, embedding_out)
      sv.saver.save(sess, FLAGS.log_dir + '/finnal-model')


if __name__ == '__main__':
  tf.app.run()
