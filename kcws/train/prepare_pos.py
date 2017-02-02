# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2017-01-25 11:46:37
# @Last Modified by:   Koth
# @Last Modified time: 2017-01-25 12:05:16

import sys
import os

totalLine = 0
longLine = 0
maxLen = 80


def processToken(token, collect, out, end):
  global totalLine
  global longLine
  global maxLen
  nn = len(token)
  #print token
  while nn > 0 and token[nn - 1] != '/':
    nn = nn - 1

  token = token[:nn - 1].strip()
  if not token:
    return
  out.write("%s " % (token))
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
  if argc < 3:
    print("Usage:%s <dir> <output>" % (argv[0]))
    sys.exit(1)
  rootDir = argv[1]
  out = open(argv[2], "w")
  for dirName, subdirList, fileList in os.walk(rootDir):
    curDir = os.path.join(rootDir, dirName)
    for file in fileList:
      if file.endswith(".txt"):
        curFile = os.path.join(curDir, file)
        # print("processing:%s" % (curFile))
        fp = open(curFile, "r")
        for line in fp.readlines():
          line = line.strip()
          processLine(line, out)
        fp.close()
  out.close()
  print("total:%d, long lines:%d" % (totalLine, longLine))


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)
