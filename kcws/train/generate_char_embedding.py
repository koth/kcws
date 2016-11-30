# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-11-30 19:59:15
# @Last Modified by:   Koth
# @Last Modified time: 2016-11-30 20:58:29
import sys
import w2v

SEQ_LEN = 5


def processFile(inp, oup, vob):
  global SEQ_LEN
  while True:
    line = inp.readline()
    if not line:
      break
    line = line.strip()
    if not line:
      continue
    ss = line.split("  ")
    x = []
    y = []
    for s in ss:
      ustr = unicode(s.decode("utf-8"))
      if len(ustr) < 1:
        continue
      nn = len(ustr)
      for i in range(nn):
        theStr = str(ustr[i].encode("utf8"))
        x.append(str(vob.GetWordIndex(theStr)))
        if i == (nn - 1):
          y.append(1)
        else:
          y.append(0)
    nn = len(x)
    for i in range(nn):
      seqLen = SEQ_LEN
      if y[i] == 1:
        seqLen = 2
      hasStop = (y[i] == 1)
      for j in range(1, seqLen):
        if (i + j + 1) > nn:
          continue
        newX = x[i:i + j + 1]
        for k in range(j + 1, SEQ_LEN):
          newX.append("0")
        newY = 0
        if y[i + j] == 1:
          if not hasStop:
            newY = 1
          hasStop = True
        line = " ".join(newX)
        line += " " + str(newY)
        oup.write("%s\n" % (line))


def main(argc, argv):
  if argc < 4:
    print("Usage: %s <input>  <output> <vec>" % (argv[0]))
    sys.exit(1)
  vob = w2v.Word2vecVocab()
  vob.Load(argv[3])
  inp = open(argv[1], "r")
  oup = open(argv[2], "w")
  processFile(inp, oup, vob)


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)