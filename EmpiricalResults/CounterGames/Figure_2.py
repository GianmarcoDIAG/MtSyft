import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

TIMEOUT = 300.0
MAX_CHAIN = 100
BASE = 80

mtsyft_1 = pd.read_csv(r"res_mtsyft.csv", header = None)
mtsyft_2 = pd.read_csv(r"res_cb_mtsyft.csv", header = None)
mtsyft_3 = pd.read_csv(r"res_conj_mtsyft.csv", header = None)

mtsyft_all = [mtsyft_1, mtsyft_2, mtsyft_3]

for mtsyft in mtsyft_all:
    mtsyft = mtsyft.reset_index()

# Figure 2 compares CSyft and refining-CSyft
df = pd.DataFrame(columns=["Number of Tiers (n)", "MtSyft", "conj-MtSyft"])
for n in range(BASE, MAX_CHAIN):
    mtsyft_1_time, mtsyft_3_time = TIMEOUT, TIMEOUT
    for i, row in mtsyft_1.iterrows():
        if "counter_1" in row[0] and "core_"+str(BASE)+"/envs_"+str(n) in row[1]:
            mtsyft_1_time = row[2]
            break
    # for i, row in csyft_2.iterrows():
    #     if "counter_8" in row[0] and "core_"+str(BASE)+"/envs_"+str(n) in row[1]:
    #         csyft_2_time = row[2]
    #         break
    for i, row in mtsyft_3.iterrows():
        if "counter_1" in row[0] and "core_"+str(BASE)+"/envs_"+str(n) in row[1]:
            mtsyft_3_time = row[2]
            break
    # print(n, mtsyft_1_time, csyft_2_time, mtsyft_3_time)
    df = df.append(
        {"Number of Tiers (n)": str(n - BASE + 1),
        "MtSyft": mtsyft_1_time,
        "conj-MtSyft": mtsyft_3_time},
        ignore_index = True
    )

print(df)

barWidth = 0.25
fig = plt.subplots()

mtsyft_1_results = df.loc[:,"MtSyft"]
mtsyft_3_results = df.loc[:,"conj-MtSyft"]

pos_bar1 = np.arange(len(mtsyft_1_results))
pos_bar2 = [x + barWidth for x in pos_bar1]

plt.bar(pos_bar1,
        mtsyft_1_results,
        width = barWidth,
        label = "MtSyft")

plt.bar(pos_bar2,
        mtsyft_3_results,
        width=barWidth,
        color = "green",
        label = "conj-MtSyft")

plt.xlabel("Number of Tiers (n)")
plt.ylabel("Time (s)")
plt.ylim(0, TIMEOUT)
plt.xticks([r + barWidth for r in range(len(mtsyft_1_results))], 
           np.arange(1, len(mtsyft_1_results)+1))
plt.legend()
plt.show()