from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import RobustScaler
from sklearn.pipeline import Pipeline
import numpy as np

import var
import common as c
from common import *


def SetupModel(max_iter: int = 1000):
    steps = [
        ("scaler", RobustScaler()),
        (
            "model",
            LogisticRegression(
                max_iter=max_iter,
                class_weight="balanced",
            ),
        ),
    ]
    var.model = Pipeline(steps)
    var.inputs = [
        "rtime_since",
        "vtime_since",
        "clock_freq",
        "lifetime_freq",
    ]
    var.input_dtype = np.float32
