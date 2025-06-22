from sklearn.linear_model import LogisticRegression
import numpy as np

import var
import common as c
from common import *


def SetupModel(max_iter: int = 1000):
    var.model = LogisticRegression(max_iter=max_iter, class_weight="balanced")
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
