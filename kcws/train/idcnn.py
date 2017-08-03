#!/usr/bin/env python
# -*- coding:utf-8 -*-

# File: idcnn.py
# Project: /Users/tech/code/kcws
# Created: Mon Jul 31 2017
# Author: Koth Chen
# Copyright (c) 2017 Koth
#
# <<licensetext>>

import tensorflow as tf


class Model:
    def __init__(self,
                 layers,
                 filterWidth,
                 numFilter,
                 embeddingDim,
                 maxSeqLen,
                 numTags,
                 repeatTimes=4):
        self.layers = layers
        self.filter_width = filterWidth
        self.num_filter = numFilter
        self.embedding_dim = embeddingDim
        self.repeat_times = repeatTimes
        self.num_tags = numTags
        self.max_seq_len = maxSeqLen

    def inference(self, X, reuse=False):
        with tf.variable_scope("idcnn", reuse=reuse):
            filter_weights = tf.get_variable(
                "idcnn_filter",
                shape=[1, self.filter_width, self.embedding_dim,
                       self.num_filter],
                initializer=tf.contrib.layers.xavier_initializer())
            layerInput = tf.nn.conv2d(X,
                                      filter_weights,
                                      strides=[1, 1, 1, 1],
                                      padding="SAME",
                                      name="init_layer")
            finalOutFromLayers = []
            totalWidthForLastDim = 0
            for j in range(self.repeat_times):
                for i in range(len(self.layers)):
                    dilation = self.layers[i]['dilation']
                    isLast = True if i == (len(self.layers) - 1) else False
                    with tf.variable_scope("atrous-conv-layer-%d" % i,
                                           reuse=True
                                           if (reuse or j > 0) else False):
                        w = tf.get_variable(
                            "filterW",
                            shape=[1, self.filter_width, self.num_filter,
                                   self.num_filter],
                            initializer=tf.contrib.layers.xavier_initializer())
                        b = tf.get_variable("filterB", shape=[self.num_filter])
                        conv = tf.nn.atrous_conv2d(layerInput,
                                                   w,
                                                   rate=dilation,
                                                   padding="SAME")
                        conv = tf.nn.bias_add(conv, b)
                        conv = tf.nn.relu(conv)
                        if isLast:
                            finalOutFromLayers.append(conv)
                            totalWidthForLastDim += self.num_filter
                        layerInput = conv
            finalOut = tf.concat(axis=3, values=finalOutFromLayers)
            keepProb = 1.0 if reuse else 0.5
            finalOut = tf.nn.dropout(finalOut, keepProb)

            finalOut = tf.squeeze(finalOut, [1])
            finalOut = tf.reshape(finalOut, [-1, totalWidthForLastDim])

            finalW = tf.get_variable(
                "finalW",
                shape=[totalWidthForLastDim, self.num_tags],
                initializer=tf.contrib.layers.xavier_initializer())

            finalB = tf.get_variable("finalB",
                                     initializer=tf.constant(
                                         0.001, shape=[self.num_tags]))

            scores = tf.nn.xw_plus_b(finalOut, finalW, finalB, name="scores")
        if reuse:
            scores = tf.reshape(scores, [-1, self.max_seq_len, self.num_tags],
                                name="Reshape_7")
        else:
            scores = tf.reshape(scores, [-1, self.max_seq_len, self.num_tags],
                                name=None)
        return scores
