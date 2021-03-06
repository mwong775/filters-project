import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import csv

rehash = []
count = []
percent_count = [] # % of buckets rehashed to specific count
cumulative = [] # number of buckets rehashed at each round
percent_cumu = [] # % of buckets rehashed at round

with open('../csv/cert_rehash_dist.csv') as file:
    reader = csv.DictReader(file, delimiter=',')
    for row in reader:
        rehash.append(int(row['rehashes']))
        count.append(int(row['buckets']))

print(rehash)
# rehash.pop(0)
# count.pop(0)
print(count)
print(sum(count))

for i in range(0, len(count)):
    c_sum = 0
    percent_count.append(count[i] * 100 / sum(count))
    if i == 0:
        continue
    for j in range(i, len(count)):
        c_sum += count[j]
    percent_cumu.append(c_sum * 100 / sum(count))
    cumulative.append(c_sum)

print(percent_count)
print(cumulative)
percent_cumu.insert(0, 0)
print(percent_cumu)

size = 20
plt.rc('axes', titlesize=size)
plt.rc('axes', labelsize=size)
plt.rcParams.update({'font.size': 12})

# fig = plt.figure()
fig, ax = plt.subplots()
ax.locator_params(integer=True)
plt.xticks(np.arange(min(rehash), max(rehash)+1, 1.0))
# rehash.pop(0)
rehash = [int(i) for i in rehash]
plt.bar(rehash, percent_cumu, color='y', label='cumulative')
plt.bar(rehash, percent_count, label='specific count')
plt.plot(loc = "upper right")
plt.xlabel("Rehash Count")
plt.ylabel("Frequency out of Total Buckets")
# plt.title("Bucket Rehashing Frequency Distribution")
ax.yaxis.set_major_formatter(mtick.PercentFormatter())
ax.legend()

"""
ax2 = ax.twinx()
ax2.bar(rehash, count)
ax2.set_ylabel("Frequency")

# xlocs, xlabs = plt.xticks()
ax.yaxis.set_major_formatter(mtick.PercentFormatter())
"""

# plt.grid(True)
plt.show()
fig.savefig("../figures/cert_rehash_dist.png")

exit()
