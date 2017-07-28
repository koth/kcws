# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-11-16 22:46:50
# @Last Modified by:   Koth
# @Last Modified time: 2016-11-21 22:40:47
import sys
import random


def main(argc, argv):
    if argc < 2:
        print("Usage:%s <input>" % (argv[0]))
        sys.exit(1)
    SENTENCE_LEN = 80
    fp = open(argv[1], "r")
    nl = 0
    bad = 0
    test = 0
    tr_p = open("train.txt", "w")
    te_p = open("test.txt", "w")
    while True:
        line = fp.readline()
        if not line:
            break
        line = line.strip()
        if not line:
            continue
        ss = line.split(' ')

        if len(ss) != (2 * SENTENCE_LEN):
            print("len is:%d" % (len(ss)))
            continue
        numV = 0
        for i in range(SENTENCE_LEN):
            if int(ss[i]) != 0:
                numV += 1
                if numV > 2:
                    break
        if numV <= 2:
            bad += 1
        else:
            r = random.random()
            if r <= 0.02 and test < 8000:
                te_p.write("%s\n" % (line))
                test += 1
            else:
                tr_p.write("%s\n" % (line))
        nl += 1
    fp.close()
    print("got bad:%d" % (bad))


if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
