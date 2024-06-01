import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

# width of bars
MIN_ROOMS = 2 
MAX_ROOMS = 10
TIMEOUT = 1000.0

mtsyft_1 = pd.read_csv(r"res_mtsyft.csv", header = None)
mtsyft_2 = pd.read_csv(r"res_cb_mtsyft.csv", header = None)
mtsfyt_3 = pd.read_csv(r"res_conj_mtsyft.csv", header = None)

mtsfyt_all = [mtsyft_1, mtsyft_2, mtsfyt_3]

for mtsfyt in mtsfyt_all:
    mtsfyt = mtsfyt.reset_index()

# constructs output table
table = pd.DataFrame(columns=["Rooms", "Number of Tiers (n)", "MtSfyt", "cb-MtSyft", "conj-MtSyft"])
for k in range(MIN_ROOMS, MAX_ROOMS+1):
    for n in range(1, k+1):
        mtsyft_1_time = TIMEOUT
        mtsyft_2_time = TIMEOUT
        mtsfyt_3_time = TIMEOUT
        for i, row in mtsyft_1.iterrows():
            if ("rooms_"+str(k)) in row[0] and ("envs_"+str(n) in row[1]):
                mtsyft_1_time = row[2]
        for i, row in mtsyft_2.iterrows():
            if ("rooms_"+str(k)) in row[0] and ("envs_"+str(n) in row[1]):
                mtsyft_2_time = row[2]
        for i, row in mtsfyt_3.iterrows():
            if ("rooms_"+str(k)) in row[0] and ("envs_"+str(n) in row[1]):
                mtsfyt_3_time = row[2]
        table = table.append(
                {'Rooms' : str(k), 
                 'Number of Tiers (n)' : str(n), 
                 'MtSfyt' : mtsyft_1_time, 
                 "cb-MtSyft": mtsyft_2_time, 
                 "conj-MtSyft": mtsfyt_3_time}, 
             ignore_index = True)

print(table)
