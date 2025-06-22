from skl2onnx import to_onnx
from skl2onnx.common.data_types import (
    Int64TensorType,
)
import joblib
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.metrics import classification_report, accuracy_score, confusion_matrix
from sklearn.model_selection import train_test_split
from typing import Union
import onnx_runtime

import var

DataType = Union[Int64TensorType]


def AddDatasets(*paths: str):
    var.datasets += paths


def ResetDatasets():
    var.datasets = []


def ZipfLoadDatasets(start: int, stop: int):
    var.df = pd.concat(
        [
            pd.read_csv(
                p,
                skipinitialspace=True,
                usecols=var.inputs
                + [
                    "wasted",
                    "obj_id",
                ],
            )
            for p in var.datasets
        ],
        ignore_index=True,
    )
    var.df = var.df.query("obj_id >= @start and obj_id < @stop")
    if "obj_id" not in var.inputs:
        var.df = var.df.drop(
            columns="obj_id",
            axis=1,
        )


def LoadDatasets():
    var.df = pd.concat(
        [
            pd.read_csv(p, skipinitialspace=True, usecols=var.inputs + ["wasted"])
            for p in var.datasets
        ],
        ignore_index=True,
    )


def SetTestData():
    if var.input_dtype is not None:
        var.X_test = np.array(
            var.df.drop("wasted", axis=1, errors="ignore"), dtype=var.input_dtype
        )
    else:
        var.X_test = np.array(var.df.drop("wasted", axis=1, errors="ignore"))
    var.y_test = np.array(var.df.wasted, dtype=np.int64)


def SetTrainData():
    if var.input_dtype is not None:
        var.X_train = np.array(
            var.df.drop("wasted", axis=1, errors="ignore"), dtype=var.input_dtype
        )
    else:
        var.X_train = np.array(var.df.drop("wasted", axis=1, errors="ignore"))
    var.y_train = np.array(var.df["wasted"], dtype=np.int64)


def SplitData():
    df = var.df
    X = None
    if var.input_dtype is not None:
        X = np.array(df.drop("wasted", axis=1, errors="ignore"), dtype=var.input_dtype)
    else:
        X = np.array(df.drop("wasted", axis=1, errors="ignore"))
    y = np.array(var.df["wasted"], dtype=np.int64)
    var.X_train, var.X_test, var.y_train, var.y_test = train_test_split(
        X, y, test_size=0.2, random_state=9, stratify=y
    )


def Train():
    if var.model is None:
        raise Exception("Model has not been set up")
    if var.model.fit is None:
        raise Exception("Model doesn't have fit method, need custom fit")
    var.model.fit(var.X_train, var.y_train)


def TrainPartial():
    if var.model is None:
        raise Exception("Model has not been set up")
    if var.model.partial_fit is None:
        raise Exception(
            "Model doesn't have partial_fit method, need custom partial_fit"
        )
    var.model.partial_fit(var.X_train, var.y_train, classes=np.array([0, 1]))


def LoadONNX(path: str):
    var.model = onnx_runtime.SklearnONNXPredictor(path)


def LoadModel(path: str = "model.pkl"):
    var.model = joblib.load(path)


def SaveModel(path: str = "model.pkl"):
    if var.model is None:
        raise Exception("Model has not been set up, exec SetupModel and Train")
    joblib.dump(var.model, path)


def ExportONNX(
    path: str = "model.onnx",
):
    if var.model is None:
        raise Exception("Model has not been set up, exec SetupModel and Train")
    if var.X_train is None:
        raise Exception("X_train has not been set up")
    onx = to_onnx(var.model, var.X_train)
    file = open(path, "wb")
    file.write(onx.SerializeToString())
    file.close()


def Test(treshold=0.5):
    if var.X_test is None:
        raise Exception("X_test has not been set up")
    if var.y_test is None:
        raise Exception("y_test has not been set up")
    if var.model is None:
        raise Exception("Model has not been set up")
    print("#### Model")
    predictions_probs = var.model.predict_proba(var.X_test)
    predictions = (predictions_probs[:, 1] >= treshold).astype(int)
    print(predictions)
    print("Classification Report:")
    print(classification_report(var.y_test, predictions))
    print("Accuracy:", accuracy_score(var.y_test, predictions))
    cm = confusion_matrix(var.y_test, predictions)
    print("Confusion Matrix:")
    print(cm)
    print("Confusion Matrix (%):")
    print(cm / cm.sum() * 100)


def PlotSave(path: str = "plot.png"):
    if var.model is None:
        raise Exception("Model has not been set up, exec SetupModel and Train")
    plt.figure(figsize=(6, 5))
    sns.heatmap(
        confusion_matrix(var.y_test, var.model.predict(var.X_test)),
        annot=True,
        fmt="d",
        cmap="Blues",
        xticklabels=["Not Wasted (0)", "Wasted (1)"],
        yticklabels=["Not Wasted (0)", "Wasted (1)"],
    )
    plt.title("Confusion Matrix")
    plt.xlabel("Predicted")
    plt.ylabel("True")
    plt.savefig(path)


def PlotShow():
    if var.model is None:
        raise Exception("Model has not been set up, exec SetupModel and Train")
    plt.figure(figsize=(6, 5))
    sns.heatmap(
        confusion_matrix(var.y_test, var.model.predict(var.X_test)),
        annot=True,
        fmt="d",
        cmap="Blues",
        xticklabels=["Not Wasted (0)", "Wasted (1)"],
        yticklabels=["Not Wasted (0)", "Wasted (1)"],
    )
    plt.title("Confusion Matrix")
    plt.xlabel("Predicted")
    plt.ylabel("True")
    plt.show()


def DescribeData():
    if var.X_train is None or var.y_train is None:
        raise Exception("Datasets has not been setup, exec SetupData")
    print("#### Datasets")
    print("\n🧾 Basic Info:")

    data = pd.concat([var.X_train, var.y_train], axis=1)
    print("-" * 60)
    print(data.info())

    print("\n📌 Missing Values:")
    print("-" * 60)
    print(data.isnull().sum())

    print("\n🆔 Unique Values per Column:")
    print("-" * 60)
    print(data.nunique())

    print("\n📊 Column-wise Summary Statistics:")
    print("-" * 60)
    for col in data.columns:
        print(f"\n🔹 Column: {col}")
        print("-" * 40)
        print(f"Type: {data[col].dtype}")
        print(f"Missing: {data[col].isnull().sum()}")
        print(f"Unique: {data[col].nunique()}")
        if pd.api.types.is_numeric_dtype(data[col]):
            print(data[col].describe())
        else:
            print(data[col].value_counts().head(10))  # show top 10 values

    if "wasted" in data.columns:
        for val in [0, 1]:
            subset = data[data["wasted"] == val]
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
