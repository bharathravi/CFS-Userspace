import sys


groups = [[], [], [], []]
total = []
if __name__ == '__main__':
  num_files = len(sys.argv)
  
  for i in range(1, num_files-1):
    f = open(sys.argv[i])
    for line in f:
      words = line.split(None)
      tid = words[1]
      gid = int(words[2])
      runtime = words[3]
      if (words[0]):
        groups[gid].append(runtime)

  for i in range(0,4):
    total = 0;
    for time in groups[i]:
      total += int(time)
    
    print 'Group ', i, ':', total, total/len(groups[i])
