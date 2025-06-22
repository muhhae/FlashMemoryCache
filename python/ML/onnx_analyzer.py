import onnx
import sys
from onnx import numpy_helper
import numpy as np


def Analyze(path: str):
    model = onnx.load(path)
    print(f"Model IR Version: {model.ir_version}")
    print(f"Producer: {model.producer_name} v{model.producer_version}")
    print(f"Domain: {model.domain}")
    print(f"Model Version: {model.model_version}")
    print(f"Graph Name: {model.graph.name}")

    print("\nInputs:")
    for input in model.graph.input:
        print(f"  {input.name}: {input.type}")

    print("\nOutputs:")
    for output in model.graph.output:
        print(f"  {output.name}: {output.type}")

    print("\nWeight:")

    for node in model.graph.node:
        if node.op_type == "LinearClassifier":
            for attr in node.attribute:
                if attr.name == "coefficients":
                    # float list
                    coeffs = np.array(attr.floats)
                    print("Coefficients:", coeffs)
                elif attr.name == "intercepts":
                    intercepts = np.array(attr.floats)
                    print("Intercepts:", intercepts)

    print("\nNodes:")
    for node in model.graph.node:
        print(f"  {node.op_type} - Inputs: {node.input} Outputs: {node.output}")


if __name__ == "__main__":
    Analyze(sys.argv[1])
