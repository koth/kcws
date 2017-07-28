#!/usr/bin/env python
# -*- coding:utf-8 -*-

# File: generate_train_free.py
# Project: /e/code/kcws
# Created: Thu Jul 27 2017
# Author: Koth Chen
# Copyright (c) 2017 Koth
#
# <<licensetext>>


import sys
import os
import w2v
import fire
from sentence import Sentence

totalLine = 0
longLine = 0

MAX_LEN = 80
totalChars = 0


def processLine(line, vob, out):
    global totalLine
    global longLine
    global totalChars
    ss = line.split("\t")

    sentence = Sentence()
    nn = len(ss)
    for i in range(nn):
        ts = ss[i].split(" ")
        ustr = unicode(ts[0].decode('utf8'))
        sentence.addToken(ustr)
    if sentence.chars > MAX_LEN:
        longLine += 1
    else:
        x = []
        y = []
        totalChars += sentence.chars
        sentence.generate_tr_line(x, y, vob)
        nn = len(x)
        assert (nn == len(y))
        for j in range(nn, MAX_LEN):
            x.append(0)
            y.append(0)
            line = ''
        for i in range(MAX_LEN):
            if i > 0:
                line += " "
            line += str(x[i])
        for j in range(MAX_LEN):
            line += " " + str(y[j])
        out.write("%s\n" % (line))
    totalLine += 1


def doGen(inputPath, outputPath, vocabPath):
    global totalLine
    global longLine
    global totalChars
    vob = w2v.Word2vecVocab()
    vob.Load(vocabPath)
    with open(inputPath, "r") as inp:
        with open(outputPath, "w") as out:
            for line in inp.readlines():
                line = line.strip()
                if not line:
                    continue
                processLine(line, vob, out)
    print("total:%d, long lines:%d, chars:%d" %
          (totalLine, longLine, totalChars))


def main():
    fire.Fire()


if __name__ == '__main__':
    main()
