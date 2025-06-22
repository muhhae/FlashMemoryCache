import pandas as pd
import sys
import math


def describe_csv(file_path):
    df = pd.read_csv(file_path)
    df.columns = df.columns.str.strip()

    print("\n🧾 Basic Info:")
    print("-" * 60)
    print(df.info())

    print("\n📌 Missing Values:")
    print("-" * 60)
    print(df.isnull().sum())

    print("\n🆔 Unique Values per Column:")
    print("-" * 60)
    print(df.nunique())

    print("\n📊 Column-wise Summary Statistics:")
    print("-" * 60)
    for col in df.columns:
        print(f"\n🔹 Column: {col}")
        print("-" * 40)
        print(f"Type: {df[col].dtype}")
        print(f"Missing: {df[col].isnull().sum()}")
        print(f"Unique: {df[col].nunique()}")
        if pd.api.types.is_numeric_dtype(df[col]):
            print(df[col].describe())
        else:
            print(df[col].value_counts().head(10))  # show top 10 values

    if "wasted" in df.columns:
        for val in [0, 1]:
            subset = df[df["wasted"] == val]
            print(f"\n{'✅' if val == 1 else '🚫'} Subset: wasted == {val}")
            print("-" * 60)
            for col in subset.columns:
                print(f"\n🔹 Column: {col}")
                print("-" * 40)
                if pd.api.types.is_numeric_dtype(subset[col]):
                    print(subset[col].describe())
                else:
                    print(subset[col].value_counts().head(10))
    else:
        print("\n⚠️ Column 'wasted' not found in CSV.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python describe_csv.py <your_file.csv>")
    else:
        describe_csv(sys.argv[1])
