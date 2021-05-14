import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import csv

ins_size = []
rehash_rds = []
rehash_total = []
bucket_avg = []
percent_rehash = []
lf = []

with open('../csv/rehash_sizes.csv') as file:
    reader = csv.DictReader(file, delimiter=',')
    labels = reader.fieldnames
    for row in reader:

        ins_size.append(int(row['insert size']))
        rehash_rds.append(int(row['rehash rounds']))
        # rehash_total.append(int(row['total rehash']))
        # bucket_avg.append(float(row['average per bucket']))
        percent_rehash.append(float(row['percent rehashed buckets']))
        lf.append(float(row['load factor']))
    
print(ins_size, bucket_avg, percent_rehash, lf)

x = np.arange(len(ins_size)) # the label locations
width = 0.35 # the width of the bars

fig, fp1 = plt.subplots()
fig.set_size_inches(8, 5.5)
fp1.bar(x - width/2, percent_rehash, width, label='Percent Rehashed Buckets')
# fp1.bar(x, bucket_avg, width, label='Average Per Bucket')
fp1.bar(x + width/2, lf, width, label='Load Factor') # x + width

fp1.yaxis.set_major_formatter(mtick.PercentFormatter())
fp1.legend()
plt.xticks(x, ins_size)
plt.xlabel("Insertion Size")
plt.ylabel("Percentage")
plt.title("Bucket Rehash Distribution Over Size")

plt.grid(True)
plt.show()
fig.savefig("../figures/rehash_sizes.png")


exit()