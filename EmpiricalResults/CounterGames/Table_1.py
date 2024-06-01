import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

TIMEOUT = 300.0
MAX_CHAIN = 100
MAX_COUNTER = 10

mtsyft_1 = pd.read_csv(r"res_mtsyft.csv", header = None)
mtsyft_2 = pd.read_csv(r"res_cb_mtsyft.csv", header = None)
mtsyft_3 = pd.read_csv(r"res_conj_mtsyft.csv", header = None)

mtsyft_all = [mtsyft_1, mtsyft_2, mtsyft_3]

for mtsyft in mtsyft_all:
    mtsyft = mtsyft.reset_index()

table = pd.DataFrame(columns=["Counter Size (k)", "Number of Environments (n)", "MtSyft", "cb-MtSyft", "conj-MtSyft"])
for k in range(1, MAX_COUNTER+1):
    print("elaborating counter size k="+str(k))
    for n in range(1, MAX_CHAIN+1):
        # print("Number of Environments:", n)
        mtsyft_1_time = TIMEOUT
        mtsyft_2_time = TIMEOUT
        mtsyft_3_time = TIMEOUT
        for i, row in mtsyft_1.iterrows():
            if "counter_"+str(k) in row[0] and "core_1/" in row[1] and row[1].endswith(str(n)):
                mtsyft_1_time = row[2]
                break
        for i, row in mtsyft_2.iterrows():
            if "counter_"+str(k) in row[0] and "core_1/" in row[1] and row[1].endswith(str(n)):
                mtsyft_2_time = row[2]
                break
        for i, row in mtsyft_3.iterrows():
            if "counter_"+str(k) in row[0] and "core_1/" in row[1] and row[1].endswith(str(n)):
                mtsyft_3_time = row[2]
                break
        table = table.append(
            {"Counter Size (k)": str(k),
            "Number of Environments (n)": str(n),
             "MtSyft": mtsyft_1_time,
             "cb-MtSyft": mtsyft_2_time, 
             "conj-MtSyft": mtsyft_3_time},
            ignore_index = True
        )

# print(table)

# for each implementation, determines the number of solved instances and average run time
print("####################### OUTPUT TABLES #######################")
out_table_1 = pd.DataFrame(columns=["Counter Size (k)", "MtSyft's Coverage", "MtSyft's Avg. RT"])
out_table_2 = pd.DataFrame(columns=["Counter Size (k)", "cb-MtSyft's Coverage", "cb-MtSyft's Avg. RT"])
out_table_3 = pd.DataFrame(columns=["Counter Size (k)", "conj-MtSyft's Coverage", "conj-MtSyft's Avg. RT"])
for k in range(1, MAX_COUNTER+1):
    mtsyft_1_solved_instances, mtsyft_1_total_runtime = 0, 0.0
    mtsyft_2_solved_instances, mtsyft_2_total_runtime = 0, 0.0
    mtsyft_3_solved_instances, mtsyft_3_total_runtime = 0, 0.0
    for i, row in table.iterrows():
        if row["Counter Size (k)"]==str(k) and row["MtSyft"] < TIMEOUT:
            mtsyft_1_solved_instances += 1
            mtsyft_1_total_runtime += row["MtSyft"]
        if row["Counter Size (k)"]==str(k) and row["cb-MtSyft"] < TIMEOUT:
            mtsyft_2_solved_instances += 1
            mtsyft_2_total_runtime += row["cb-MtSyft"]
        if row["Counter Size (k)"]==str(k) and row["conj-MtSyft"] < TIMEOUT:
            mtsyft_3_solved_instances += 1
            mtsyft_3_total_runtime += row["conj-MtSyft"]
        avg_mtsyft_1, avg_mtsyft_2, avg_mtsyft_3 = 0.0, 0.0, 0.0
        if (mtsyft_1_solved_instances != 0):
            avg_mtsyft_1 = mtsyft_1_total_runtime / mtsyft_1_solved_instances
        if (mtsyft_2_solved_instances != 0):
            avg_mtsyft_2 = mtsyft_2_total_runtime / mtsyft_2_solved_instances
        if (mtsyft_3_solved_instances != 0):
            avg_mtsyft_3 = mtsyft_3_total_runtime / mtsyft_3_solved_instances
    out_table_1 = out_table_1.append(
        {'Counter Size (k)' : str(k), 
         "MtSyft's Coverage" : mtsyft_1_solved_instances,
         "MtSyft's Avg. RT": avg_mtsyft_1
        }, ignore_index = True
    )
    out_table_2 = out_table_2.append(
        {'Counter Size (k)' : str(k), 
         "cb-MtSyft's Coverage" : mtsyft_2_solved_instances,
         "cb-MtSyft's Avg. RT": avg_mtsyft_2
        }, ignore_index = True
    )
    out_table_3 = out_table_3.append(
        {'Counter Size (k)' : str(k), 
         "conj-MtSyft's Coverage" : mtsyft_3_solved_instances,
         "conj-MtSyft's Avg. RT": avg_mtsyft_3
        }, ignore_index = True
    )
    
print(out_table_1)
print(out_table_2)
print(out_table_3)