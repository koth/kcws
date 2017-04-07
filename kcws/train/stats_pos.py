# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2017-01-25 14:55:00
# @Last Modified by:   Koth
# @Last Modified time: 2017-04-07 22:12:33

import sys
import os

totalLine = 0
longLine = 0
maxLen = 80
posMap = {}


def processToken(token, collect, out, end):
    global totalLine
    global longLine
    global maxLen
    global posMap
    nn = len(token)
    oline = token
    while nn > 0 and token[nn - 1] != '/':
        nn = nn - 1
    pos = token[nn:]
    token = token[:nn - 1].strip()
    if not token:
        return
    if (not pos[0:1].isalpha()) or pos[0:1].isupper():
        return
    if len(pos) > 2:
        pos = pos[:2]
    posMap.setdefault(pos, 0)
    posMap[pos] += 1
    out.write("%s %s\t" % (token, pos))
    if end:
        out.write("\n")


def processLine(line, out):
    line = line.strip()
    nn = len(line)
    seeLeftB = False
    start = 0
    collect = []
    try:
        for i in range(nn):
            if line[i] == ' ':
                if not seeLeftB:
                    token = line[start:i]
                    if token.startswith('['):
                        tokenLen = len(token)
                        while tokenLen > 0 and token[tokenLen - 1] != ']':
                            tokenLen = tokenLen - 1
                        token = token[1:tokenLen - 1]
                        ss = token.split(' ')
                        for s in ss:
                            processToken(s, collect, out, False)
                    else:
                        processToken(token, collect, out, False)
                    start = i + 1
            elif line[i] == '[':
                seeLeftB = True
            elif line[i] == ']':
                seeLeftB = False
        if start < nn:
            token = line[start:]
            if token.startswith('['):
                tokenLen = len(token)
                while tokenLen > 0 and token[tokenLen - 1] != ']':
                    tokenLen = tokenLen - 1
                token = token[1:tokenLen - 1]
                ss = token.split(' ')
                ns = len(ss)
                for i in range(ns - 1):
                    processToken(ss[i], collect, out, False)
                processToken(ss[-1], collect, out, True)
            else:
                processToken(token, collect, out, True)
    except Exception as e:
        pass


def main(argc, argv):
    global totalLine
    global longLine
    global posMap
    if argc < 4:
        print("Usage:%s <dir> <pos_vob_out> <for_train_out>" % (argv[0]))
        sys.exit(1)
    rootDir = argv[1]
    out = open(argv[3], "w")
    tagvobFp = open(argv[2], "w")
    for dirName, subdirList, fileList in os.walk(rootDir):
        curDir = os.path.join(rootDir, dirName)
        for file in fileList:
            if file.endswith(".txt"):
                curFile = os.path.join(curDir, file)
                fp = open(curFile, "r")
                for line in fp.readlines():
                    line = line.strip()
                    processLine(line, out)
                fp.close()
    out.close()
    print("total:%d, long lines:%d" % (totalLine, longLine))
    print("total pos tags:%d" % (len(posMap)))
    idx = 0
    for k, v in posMap.iteritems():
        tagvobFp.write("%s\t%d\n" % (k, idx + 1))
        idx += 1


if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
