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
                random_state=42,
            ),
        ),
    ]
    var.model = Pipeline(steps)
    var.inputs = [
        "clock_freq_decayed_rtime",
    ]
    var.input_dtype = np.float32
