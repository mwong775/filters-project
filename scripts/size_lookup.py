import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import csv

num_items = []
bfc_neg_tput = []
bfc_pos_tput = []
vp_neg_tput = []
vp_pos_tput = []


bfc_size = []
bfc_bits_per_item = []
vp_size = []
vp_bits_per_item = []


# rows = ('num items', 'bfc neg', 'bfc pos', 'total size', 'bits per item')

# bfc data
with open('../bfc_size_lookup.csv') as file:
    reader = csv.DictReader(file, delimiter=',')
    for row in reader:
        num_items.append(int(row['num items']))
        bfc_neg_tput.append(float(row[' bfc neg']))
        bfc_pos_tput.append(float(row[' bfc pos']))
        bfc_size.append(int(row[' total size']))
        bfc_bits_per_item.append(float(row[' bits per item']))

# vp data
with open('../vp_size_lookup.csv') as file:
    reader = csv.DictReader(file, delimiter=',')
    for row in reader:
        vp_neg_tput.append(float(row[' vp neg']))
        vp_pos_tput.append(float(row[' vp pos']))
        vp_size.append(int(row[' total size']))
        vp_bits_per_item.append(float(row[' bits per item']))

# convert size from bytes to MB (divide by 1 mil)
bfc_size = [s / 10**6 for s in bfc_size]
vp_size = [s / 10**6 for s in vp_size]

plt.rcParams.update({'font.size': 14})

# Positive + Negative lookup throughput
fig, fp1 = plt.subplots()
fp1.plot(num_items, bfc_pos_tput, marker = 'o', label="BFC pos")
fp1.plot(num_items, bfc_neg_tput, marker = 'o', label="BFC neg")
fp1.plot(num_items, vp_pos_tput, marker = 'o', label="VP pos")
fp1.plot(num_items, vp_neg_tput, marker = 'o', label="VP neg")
fp1.legend(loc = "upper right")
xticklabels = ['{:,}'.format(int(x)) + 'K' for x in fp1.get_xticks()/1000]
fp1.set_xticklabels(xticklabels)
fp1.set_xlabel("Number of items") #  $\mathregular{10^8}$
fp1.set_ylabel("Lookup throughput (MOPS)")

# plt.grid(True)
plt.show()
fig.savefig("../figures/size_lookup.png")

plt.rcParams.update({'font.size': 18})

# total memory/size
fig, fp1 = plt.subplots()
fp1.plot(num_items, bfc_size, marker = 'o', label="BFC")
fp1.plot(num_items, vp_size, marker = 'o', label="VP")
fp1.legend(loc = "upper left")
xticklabels = ['{:,}'.format(int(x)) + 'K' for x in fp1.get_xticks()/1000]
fp1.set_xticklabels(xticklabels)
fp1.set_xlabel("Number of items") #  $\mathregular{10^8}$
fp1.set_ylabel("Memory cost (MB)")

# plt.grid(True)
plt.show()
fig.savefig("../figures/size_totalmem_lookup.png")

# bits per item
fig, fp1 = plt.subplots()
fp1.plot(num_items, bfc_bits_per_item, marker = 'o', label="BFC")
fp1.plot(num_items, vp_bits_per_item, marker = 'o', label="VP")
fp1.legend(loc = "upper right")
xticklabels = ['{:,}'.format(int(x)) + 'K' for x in fp1.get_xticks()/1000]
fp1.set_xticklabels(xticklabels)
fp1.set_xlabel("Number of items")
fp1.set_ylabel("Bits per item")

# plt.grid(True)
plt.show()
fig.savefig("../figures/size_bits_lookup.png")

# exit()