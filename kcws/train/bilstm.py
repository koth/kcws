#!/usr/bin/env python
# -*- coding:utf-8 -*-

# File: bilstm.py
# Project: /e/code/kcws
# Created: Thu Aug 03 2017
# Author: Koth Chen
# Copyright (c) 2017 Koth
#
# <<licensetext>>

import tensorflow as tf


class Model:
    def __init__(self,
                 numHidden,
                 maxSeqLen,
                 numTags):
        self.num_hidden = numHidden
        self.num_tags = numTags
        self.max_seq_len = maxSeqLen
        self.W = tf.get_variable(
            shape=[numHidden * 2, numTags],
            initializer=tf.contrib.layers.xavier_initializer(),
            name="weights",
            regularizer=tf.contrib.layers.l2_regularizer(0.001))
        self.b = tf.Variable(tf.zeros([numTags], name="bias"))

    def inference(self, X, length, reuse=False):
        length_64 = tf.cast(length, tf.int64)
        with tf.variable_scope("bilstm", reuse=reuse):
            forward_output, _ = tf.nn.dynamic_rnn(
                tf.contrib.rnn.LSTMCell(self.num_hidden,
                                        reuse=reuse),
                X,
                dtype=tf.float32,
                sequence_length=length,
                scope="RNN_forward")
            backward_output_, _ = tf.nn.dynamic_rnn(
                tf.contrib.rnn.LSTMCell(self.num_hidden,
                                        reuse=reuse),
                inputs=tf.reverse_sequence(X,
                                           length_64,
                                           seq_dim=1),
                dtype=tf.float32,
                sequence_length=length,
                scope="RNN_backword")

        backward_output = tf.reverse_sequence(backward_output_,
                                              length_64,
                                              seq_dim=1)

        output = tf.concat([forward_output, backward_output], 2)
        output = tf.reshape(output, [-1, self.num_hidden * 2])
        if reuse is None or not reuse:
            output = tf.nn.dropout(output, 0.5)

        matricized_unary_scores = tf.matmul(output, self.W) + self.b
        unary_scores = tf.reshape(
            matricized_unary_scores,
            [-1, self.max_seq_len, self.num_tags],
            name="Reshape_7" if reuse else None)
        return unary_scores
