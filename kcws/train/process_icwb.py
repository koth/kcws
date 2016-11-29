# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-11-27 12:01:18
# @Last Modified by:   Koth
# @Last Modified time: 2016-11-27 20:26:31
import sys
import w2v

SEQ_LEN = 80


def processToken(x, y, tok, vob):
  if len(tok) == 1:
    x.append(vob.GetWordIndex(str(tok[0].encode("utf8"))))
    y.append(0)
  else:
    nn = len(tok)
    for i in range(nn):
      x.append(vob.GetWordIndex(str(tok[i].encode("utf8"))))
      if i == 0:
        y.append(1)
      elif i == (nn - 1):
        y.append(3)
      else:
        y.append(2)


def processFile(inp, oup, mode, vob):
  global SEQ_LEN
  while True:
    line = inp.readline()
    if not line:
      break
    line = line.strip()
    if not line:
      continue
    ss = line.split("  ")
    oline = ""
    x = []
    y = []
    for s in ss:
      ustr = unicode(s.decode("utf-8"))
      if len(ustr) < 1:
        continue
      if mode == 0:
        for i in range(len(ustr)):
          oline += str(ustr[i].encode("utf8"))
          oline += " "
      else:
        processToken(x, y, ustr, vob)
    if mode != 0:
      nn = len(x)
      for i in range(nn, SEQ_LEN):
        x.append(0)
        y.append(0)
      for i in range(SEQ_LEN):
        oline += str(x[i]) + " "
      for i in range(SEQ_LEN):
        oline += str(y[i]) + " "
    olen = len(oline)
    oline = oline[:olen - 1]
    oup.write("%s\n" % (oline))


def main(argc, argv):
  if argc < 3:
    print(
        "Usage: %s <input>  <output> [model | 0 for w2v , 1 for training]  [vec_path | if mode if not 0]"
        % (argv[0]))
    sys.exit(1)
  mode = 0
  vob = None
  if argc > 4:
    mode = int(argv[3])
    vob = w2v.Word2vecVocab()
    vob.Load(argv[4])
  inp = open(argv[1], "r")
  oup = open(argv[2], "w")
  processFile(inp, oup, mode, vob)


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)