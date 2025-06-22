import pandas as pd
import numpy as np
from sklearn.base import BaseEstimator
from typing import List

df: pd.DataFrame = None
datasets: List[str] = []

model: BaseEstimator = None

X_train: pd.DataFrame = None
y_train: pd.DataFrame = None

X_test: pd.DataFrame = None
y_test: pd.DataFrame = None

dummy_input = None
inputs: List[str] = None
input_dtype: np.dtype = None
