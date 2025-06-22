import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from dataclasses import dataclass
import gc
import random


def sample_csv(filename, sample_size=1000):
    sample = []
    chunk_iter = pd.read_csv(filename, chunksize=10000)

    seen = 0
    for chunk in chunk_iter:
        for _, row in chunk.iterrows():
            seen += 1
            if len(sample) < sample_size:
                sample.append(row)
            else:
                r = random.randint(0, seen - 1)
                if r < sample_size:
                    sample[r] = row
    return pd.DataFrame(sample)


def fast_sample_csv(file_path, sample_size=1000, chunksize=100_000):
    columns = ["second_to_last", "second_to_last_is_wasted", "last", "last_is_wasted"]
    sampled_chunks = []

    for chunk in pd.read_csv(file_path, names=columns, skiprows=1, chunksize=chunksize):
        sampled = chunk.sample(frac=sample_size / 1_000_000, random_state=42)
        sampled_chunks.append(sampled)

        # Stop early if we’ve collected enough
        if sum(len(c) for c in sampled_chunks) >= sample_size:
            break

    result = (
        pd.concat(sampled_chunks)
        .sample(n=sample_size, random_state=42)
        .reset_index(drop=True)
    )
    return result


# second_to_last_promotion,second_to_last_promotion_is_wasted,last_promotion,last_promotion_is_wasted
# df = fast_sample_csv("../result/datasets/cluster50[0.1].csv", 1000000)
df = pd.read_csv(
    "../result/datasets/cluster50[0.1].csv",
    names=["second_to_last", "second_to_last_is_wasted", "last", "last_is_wasted"],
    skiprows=1,
)
df["gap"] = df["last"] - df["second_to_last"]
print(df["last_is_wasted"].value_counts())
print(df[df["last_is_wasted"] == 1]["gap"].describe())
print(df[df["last_is_wasted"] == 0]["gap"].describe())

plt.figure(figsize=(8, 6))
plt.scatter(
    df["gap"],
    df["last_is_wasted"],
    c=df["last_is_wasted"],
    cmap="coolwarm",
    s=100,
)

plt.xlabel("Gap Between Promotions")
plt.ylabel("Last Promotion Wasted (0 = No, 1 = Yes)")
plt.title("Gap Between Promotions vs. Last Promotion Wasted")
plt.grid(True)
plt.yticks([0, 1], ["No", "Yes"])
plt.show()
