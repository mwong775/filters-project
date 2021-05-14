import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import csv

occ = []
max_rh = []
avg_rh = []
neg_tput = []
pos_tput = []

with open('../vp_lf_lookup.csv') as file:
    reader = csv.DictReader(file, delimiter=',')
    for row in reader:
        occ.append(float(row['occupancy']))
        max_rh.append(int(row[' max rehash']))
        avg_rh.append(float(row[' avg rehash']))
        neg_tput.append(float(row[' vp neg']))
        pos_tput.append(float(row[' vp pos']))

# max rehash
fig, fp1 = plt.subplots()
fp1.plot(occ, max_rh, marker = 'o', label="max # rehash")
fp1.legend(loc = "upper right")

fp1.set_xlabel("Table occupancy")
fp1.set_ylabel("Maximum # of rehashes")

# plt.grid(True)
plt.show()
fig.savefig("../figures/lf_max_rh_lookup.png")

# average rehash
fig, fp1 = plt.subplots()
fp1.plot(occ, avg_rh, marker = 'o', label="avg. # rehash")
fp1.legend(loc = "upper right")

fp1.set_xlabel("Table occupancy")
fp1.set_ylabel("Average # of rehashes per bucket")

# plt.grid(True)
plt.show()
fig.savefig("../figures/lf_avg_rh_lookup.png")

# positive + negative lookup throughput
fig, fp1 = plt.subplots()
fp1.plot(occ, pos_tput, marker = 'o', label="pos tput")
fp1.plot(occ, neg_tput, marker = 'o', label="neg tput")

fp1.legend(loc = "upper right")

fp1.set_xlabel("Table occupancy")
fp1.set_ylabel("Lookup throughput (MOPS)")

# plt.grid(True)
plt.show()
fig.savefig("../figures/lf_lookup.png")
