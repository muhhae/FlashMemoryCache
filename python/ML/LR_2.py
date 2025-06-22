from sklearn.linear_model import LogisticRegression
import numpy as np

import var
import common as c
from common import *


def SetupModel(max_iter: int = 1000):
    var.model = LogisticRegression(max_iter=max_iter, class_weight="balanced")
    var.inputs = [
        "vtime_since",
        "clock_freq",
    ]
    var.input_dtype = np.float32
