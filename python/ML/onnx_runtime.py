import onnxruntime as ort
import var
import numpy as np


class SklearnONNXPredictor:
    def __init__(self, model_path):
        self.session = ort.InferenceSession(model_path)
        self.input_name = self.session.get_inputs()[0].name

    def predict(self, X):
        if hasattr(X, "to_numpy"):
            X = X.to_numpy()
        X = X.astype(var.input_dtype)
        outputs = self.session.run(None, {self.input_name: X})
        return outputs[0]

    def predict_proba(self, X):
        if hasattr(X, "to_numpy"):
            X = X.to_numpy()
        X = X.astype(var.input_dtype)
        outputs = self.session.run(None, {self.input_name: X})
        c_0 = [d[0] for d in outputs[1]]
        c_1 = [d[1] for d in outputs[1]]
        probs = np.column_stack((c_0, c_1))
        return probs
