import sys
import math

groups = [[], [], [], []]
totals = [[], [], [], []]
total = []
if __name__ == '__main__':
  num_files = len(sys.argv)
  for i in range(1, num_files):
    print i
    f = open(sys.argv[i])
    for line in f:
      words = line.split(None)
      tid = words[1]
      gid = int(words[2])
      runtime = words[3]
      if (words[0] == '1'):
        groups[gid].append(runtime)
      if (words[0] == '0'):
        totals[gid].append(runtime)


  # Calculate means
  ind_mean = [0,0,0,0]
  total_mean = [0,0,0,0]
  for i in range(0,4):
    ind = 0;
    for time in groups[i]:
      ind += float(time)
    total = 0;
    for time in totals[i]:
      total += float(time)
   
    ind_mean[i] = ind/len(groups[i])
    total_mean[i] = ind/len(totals[i])

  # Calculate std-dev
  ind_std = [0,0,0,0]
  total_std = [0,0,0,0]
  group = [0,0,0,0]
  for i in range(0,4):
    ind = 0;
    for time in groups[i]:
      ind += math.pow(float(time) - ind_mean[i], 2)
    total = 0;
    for time in totals[i]:
      total += math.pow(float(time) - total_mean[i], 2)

    ind_std[i] = math.sqrt(ind/len(groups[i]))
    total_std[i] = math.sqrt(total/len(totals[i]))
    group[i] = total

  print 'Group,  Group-Total, Ind (Mean), Ind(Std), Total(Mean), Total(Std)'
  for i in range(0,4):
    print 'Group ', i, ':',group[i]/1000, ind_mean[i]/1000, ind_std[i]/1000, total_mean[i]/1000, total_std[i]/1000
