import onnx
from onnx import TensorProto, numpy_helper


def fix_tensor_elem_type(type_proto):
    """Recursively convert double -> float in all ONNX type structures."""
    if type_proto.HasField("tensor_type"):
        if type_proto.tensor_type.elem_type == TensorProto.DOUBLE:
            type_proto.tensor_type.elem_type = TensorProto.FLOAT
    elif type_proto.HasField("sequence_type"):
        fix_tensor_elem_type(type_proto.sequence_type.elem_type)
    elif type_proto.HasField("map_type"):
        fix_tensor_elem_type(type_proto.map_type.value_type)


def fix_model_dtype(path_in, path_out):
    model = onnx.load(path_in)

    # Fix graph inputs
    for input_tensor in model.graph.input:
        fix_tensor_elem_type(input_tensor.type)

    # Fix graph outputs
    for output_tensor in model.graph.output:
        fix_tensor_elem_type(output_tensor.type)

    # Fix value_info (intermediate values)
    for value_info in model.graph.value_info:
        fix_tensor_elem_type(value_info.type)

    # Fix initializers (constants, weights)
    for initializer in model.graph.initializer:
        if initializer.data_type == TensorProto.DOUBLE:
            array = numpy_helper.to_array(initializer).astype("float32")
            new_initializer = numpy_helper.from_array(array, name=initializer.name)
            initializer.CopyFrom(new_initializer)

    onnx.save(model, path_out)
    onnx.checker.check_model(model)
    print(f"Fixed model saved to: {path_out}")
