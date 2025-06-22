from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import Pipeline
import numpy as np

import var
import common as c
from common import *


def SetupModel(max_iter: int = 1000):
    steps = [
        ("scaler", StandardScaler()),
        (
            "model",
            LogisticRegression(
                max_iter=max_iter,
            ),
        ),
    ]
    var.model = Pipeline(steps)
    var.inputs = [
        "rtime_since",
        "vtime_since",
        "rtime_between",
        "clock_freq",
        "lifetime_freq",
    ]
    var.input_dtype = np.float32


def SetupData():
    c.SetupData()
