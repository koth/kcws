# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-12-01 09:30:11
# @Last Modified by:   Koth
# @Last Modified time: 2016-12-01 10:19:15
import sys
import random


def main(argc, argv):
  if argc < 2:
    print("Usage: %s <input>" % (argv[0]))
    sys.exit(1)
  inp = open(argv[1], "r")
  trp = open("train.txt", "w")
  tep = open("test.txt", "w")
  sampleNum = 5000
  if argc > 2:
    sampleNum = int(argv[2])
  allf = []
  allp = []
  nf = 0
  np = 0
  while True:
    line = inp.readline()
    if not line:
      break
    line = line.strip()
    if not line:
      continue
    ss = line.split(" ")
    assert (len(ss) == 6)
    if int(ss[5]) == 0:
      nf += 1
      if len(allf) < sampleNum:
        allf.append(line)
      else:
        k = random.randint(0, nf - 1)
        if k < sampleNum:
          trp.write("%s\n" % (allf[k]))
          allf[k] = line
        else:
          trp.write("%s\n" % (line))
    else:
      np += 1
      if len(allp) < sampleNum:
        allp.append(line)
      else:
        k = random.randint(0, np - 1)
        if k < sampleNum:
          trp.write("%s\n" % (allp[k]))
          allp[k] = line
        else:
          trp.write("%s\n" % (line))
  for s in allp:
    tep.write("%s\n" % (s))
  for s in allf:
    tep.write("%s\n" % (s))


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)