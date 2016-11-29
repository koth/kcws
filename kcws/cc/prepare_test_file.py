# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-11-22 21:20:59
# @Last Modified by:   Koth
# @Last Modified time: 2016-11-22 21:39:22

import sys
import os


def main(argc, argv):
    if argc < 3:
        print("Usage:%s <input> <output>" % (argv[0]))
        sys.exit(1)
    inp = open(argv[1], "r")
    oup = open(argv[2], "w")
    totalLine = 0
    while True:
        line = inp.readline()
        if not line:
            break
        line = line.strip()
        if not line or len(line) == 0:
            continue
        ustr = unicode(line.decode("utf8"))
        if len(ustr) >= 80 or len(ustr) < 10:
            continue
        oup.write("%s\n" % (line))
        totalLine += 1
    print("totalLine:%d" % (totalLine))


if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
