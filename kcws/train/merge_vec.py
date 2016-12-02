# -*- coding: utf-8 -*-
# @Author: Koth
# @Date:   2016-12-02 13:02:30
# @Last Modified by:   Koth
# @Last Modified time: 2016-12-02 13:35:42
import sys


def main(argc, argv):
  if argc < 3:
    print("Usage:%s <w2v> <glove>" % (argv[0]))
    sys.exit(1)
  inwp = open(argv[1], "r")
  ingp = open(argv[2], "r")
  oup = open("merged_vec.txt", "w")
  inwp.readline()
  fmap = {}
  n1 = 0
  n2 = 0
  k1 = -1
  k2 = -1
  while True:
    line = inwp.readline()
    if not line:
      break
    n1 += 1
    line = line.strip()
    ss = line.split(' ')
    nn = len(ss)
    if k1 == -1:
      k1 = nn - 1
    else:
      assert (k1 == (nn - 1))
    if ss[0] == '</s>':
      ss[0] = '<unk>'
    fv = " ".join(ss[1:])
    fmap[ss[0]] = fv
  while True:
    line = ingp.readline()
    if not line:
      break
    n2 += 1
    line = line.strip()
    ss = line.split(' ')
    nn = len(ss)
    if k2 == -1:
      k2 = nn - 1
    else:
      assert (k2 == (nn - 1))
    assert (ss[0] in fmap)
    fv = " ".join(ss[1:])
    fmap[ss[0]] += " " + fv
  assert (n1 == n2)
  oup.write("%d %d\n" % (n1, k1 + k2))
  fv = fmap["<unk>"]
  oup.write("<unk> %s\n" % (fv))
  for k, v in fmap.iteritems():
    if k == '<unk>':
      continue
    oup.write("%s %s\n" % (k, v))
  oup.close()


if __name__ == '__main__':
  main(len(sys.argv), sys.argv)
