# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-12-09 19:37:43
# @Last Modified by:   Koth
# @Last Modified time: 2016-12-09 19:49:37
import sys


def main(argc, argv):
  if argc < 4:
    print("Usage:%s <vob> <input> <output>" % (argv[0]))
    sys.exit(1)
  vp = open(argv[1], "r")
  inp = open(argv[2], "r")
  oup = open(argv[3], "w")
  vobsMap = {}
  for line in vp:
    line = line.strip()
    ss = line.split(" ")
    vobsMap[ss[0]] = 1
  while True:
    line = inp.readline()
    if not line:
      break
    line = line.strip()
    if not line:
      continue
    ss = line.split(" ")
    tokens = []
    for s in ss:
      if s in vobsMap:
        tokens.append(s)
      else:
        tokens.append("<UNK>")
    oup.write("%s\n" % (" ".join(tokens)))
  oup.close()
  inp.close()
  vp.close()


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)