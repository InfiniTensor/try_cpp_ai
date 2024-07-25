import mmap
import re
import argparse
from onnx import TensorProto, NodeProto, save_model
from onnx.helper import (
    make_model,
    make_node,
    make_graph,
    make_tensor_value_info,
    make_tensor,
    make_opsetid,
)
from onnx.checker import check_model


class Topo:
    def __init__(self, bytes: bytes):
        list = bytes.strip().split(b"<-")
        self.inputs = [int(s.strip(b"%")) for s in list[1].split()]
        self.outputs = [int(s.strip(b"%")) for s in list[0].split()]

    def __str__(self) -> str:
        return f"{self.inputs} <- {self.outputs}"


class Tensor:
    def __init__(self, bytes_: bytes):
        list = bytes_.split(b"\t")
        self.name = str(list[1].strip(), "utf-8")

        def map_dt(dt: bytes) -> TensorProto.DataType:
            match dt:
                case b"F32":
                    return TensorProto.FLOAT
                case b"U8":
                    return TensorProto.UINT8
                case b"I8":
                    return TensorProto.INT8
                case b"U16":
                    return TensorProto.UINT16
                case b"I16":
                    return TensorProto.INT16
                case b"I32":
                    return TensorProto.INT32
                case b"I64":
                    return TensorProto.INT64
                case b"String":
                    return TensorProto.STRING
                case b"Bool":
                    return TensorProto.BOOL
                case b"FP16":
                    return TensorProto.FLOAT16
                case b"F64":
                    return TensorProto.DOUBLE
                case b"U32":
                    return TensorProto.UINT32
                case b"U64":
                    return TensorProto.UINT64
                case b"Complex64":
                    return TensorProto.COMPLEX64
                case b"Complex128":
                    return TensorProto.COMPLEX128
                case b"BF16":
                    return TensorProto.BFLOAT16
                case _:
                    return TensorProto.UNDEFINED

        self.dt = map_dt(list[2].strip())
        layout = list[3].strip()
        if layout != b"NCHW" and layout != b"ELSE":
            raise ValueError("Unsupported layout")
        range = list[4].strip().split()
        self.offset = int(range[0], 0)
        self.size = int(range[1], 0)
        self.shape = [int(s) for s in split_array(list[5])]

    def __str__(self) -> str:
        return f"{self.name} (dt = {self.dt}) {self.shape} {self.offset}..{self.offset + self.size}"


class Operator:
    def __init__(self, bytes: bytes):
        list = bytes.split(b"\t")
        self.name = str(list[1].strip(), "utf-8")
        list = list[2].split(b"(", 1)
        self.type = str(list[0].strip(), "utf-8")
        list = list[1].rsplit(b")", 1)
        self.meta = list[0].strip()
        self.topo = Topo(list[1])

    def __str__(self) -> str:
        return f"{self.type}: {self.name}, meta = {self.meta}, topo = {self.topo}"

    def to_node(self, tensors: list[Tensor]) -> tuple[NodeProto, list[TensorProto]]:
        DEFAULT_DOMAIN = ""
        if self.type == "BatchNormalization":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    epsilon=float(self.meta.split(b"=")[0]),
                ),
                [],
            )
        if self.type == "Conv":
            meta = [int(x) for x in split_array(self.meta)]
            rank = int(len(meta) / 4)
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    dilations=meta[0:rank],
                    strides=meta[rank : 2 * rank],
                    pads=meta[2 * rank : 4 * rank],
                ),
                [],
            )
        if self.type in ["Relu", "Tanh", "Erf", "Max"]:
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                ),
                [],
            )
        if self.type == "MaxPool":
            meta = self.meta.split(b",")
            ceil_mode = (
                1 if meta[0] == b"true" else (0 if meta[0] == b"false" else None)
            )
            kernel_shape = [int(x) for x in split_array(meta[1])]
            meta = [int(x) for x in split_array(meta[2])]
            rank = int(len(meta) / 4)
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    ceil_mode=ceil_mode,
                    kernel_shape=kernel_shape,
                    dilations=meta[0:rank],
                    strides=meta[rank : 2 * rank],
                    pads=meta[2 * rank : 4 * rank],
                ),
                [],
            )
        if self.type in [
            "Add",
            "Pow",
            "Sqrt",
            "Div",
            "Mul",
            "Sub",
            "Exp",
            "Log",
            "Neg",
            "Sigmoid",
            "Where",
        ]:
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                ),
                [],
            )
        if self.type == "GlobalAveragePool":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                ),
                [],
            )
        if self.type == "MatMul":
            meta = self.meta.split(b",")
            alpha = float(meta[0].split(b"=")[0].strip())
            beta = float(meta[1].split(b"=")[0].strip())
            transA = 1 if meta[2].strip() == b"AT" else 0
            transB = 1 if meta[3].strip() == b"BT" else 0
            if alpha != 1 or beta != 0 or transA == 1 or transB == 1:
                return (
                    make_node(
                        "Gemm",
                        [tensors[i].name for i in self.topo.inputs],
                        [tensors[i].name for i in self.topo.outputs],
                        self.name,
                        domain=DEFAULT_DOMAIN,
                        alpha=alpha,
                        beta=beta,
                        transA=transA,
                        transB=transB,
                    ),
                    [],
                )
            else:
                return (
                    make_node(
                        self.type,
                        [tensors[i].name for i in self.topo.inputs],
                        [tensors[i].name for i in self.topo.outputs],
                        self.name,
                        domain=DEFAULT_DOMAIN,
                    ),
                    [],
                )
        if self.type == "Reshape" or self.type == "Identity":
            output = tensors[self.topo.outputs[0]]
            shape_name = f"{output.name}_shape"
            shape = output.shape
            shape = make_tensor(shape_name, TensorProto.INT64, [len(shape)], shape)
            return (
                make_node(
                    "Reshape",
                    [tensors[self.topo.inputs[0]].name, shape_name],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                ),
                [shape],
            )
        if self.type in ["Gather", "Concat", "Softmax", "Split"]:
            meta = self.meta.split(b"/")
            axis = int(meta[0])
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    axis=axis,
                ),
                [],
            )
        if self.type == "ReduceMean":
            meta = self.meta.split(b",")
            keepDims = meta[2] == b" true"
            axes = [int(x) for x in split_array(meta[0])]
            return (
                make_node(
                    self.type,
                    [tensors[self.topo.inputs[0]].name],
                    [tensors[self.topo.outputs[0]].name],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    axes=axes,
                    keepdims=keepDims,
                ),
                [],
            )
        if self.type == "Transpose":
            meta = [int(x) for x in split_array(self.meta)]
            return (
                make_node(
                    self.type,
                    [tensors[self.topo.inputs[0]].name],
                    [tensors[self.topo.outputs[0]].name],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    perm=meta,
                ),
                [],
            )
        if self.type == "Slice":
            # starts, ends, axes, steps = split_array_slice(self.meta)
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[self.topo.outputs[0]].name],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                ),
                [],
            )
        if self.type == "Cast":
            to = int(tensors[self.topo.outputs[0]].dt)
            return (
                make_node(
                    self.type,
                    [tensors[self.topo.inputs[0]].name],
                    [tensors[self.topo.outputs[0]].name],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    to=to,
                ),
                [],
            )
        if self.type == "RmsNormalization":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain="refactor",
                    epsilon=float(self.meta.split(b"=")[0]),
                ),
                [],
            )
        if self.type == "LayerNormalization":
            meta = self.meta.split(b",")
            epsilon = float(meta[0].split(b"=")[0].strip())
            axis = int(meta[1])
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain="refactor",
                    epsilon=epsilon,
                    axis=axis,
                ),
                [],
            )
        if self.type == "RotaryPositionEmbedding":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain="refactor",
                    theta=float(self.meta.split(b"=")[0]),
                ),
                [],
            )
        if self.type == "Gelu":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain="refactor",
                ),
                [],
            )
        if self.type == "LeakyRelu":
            return (
                make_node(
                    self.type,
                    [tensors[i].name for i in self.topo.inputs],
                    [tensors[i].name for i in self.topo.outputs],
                    self.name,
                    domain=DEFAULT_DOMAIN,
                    alpha=float(self.meta),
                ),
                [],
            )

        raise ValueError(f"Unsupported operator {self.type}")


def parse_args():
    parser = argparse.ArgumentParser(description="Analysis serialize file.")
    parser.add_argument(
        "--input",
        type=str,
        default="./",
        help="Path to save the serialize output files.",
    )
    args = parser.parse_args()
    return args.input


def split_array(arr: bytes):
    return (x for x in arr.strip().strip(b"[").strip(b"]").split())


def split_array_slice(arr: bytes):
    meta_array = split_array(arr)
    meta = [list(map(int, re.findall(r"\d+", x))) for x in meta_array]
    starts = [int(x[0]) for x in meta]
    ends = [int(x[0] + x[1] * x[2]) for x in meta]
    axes = [x for x in range(len(meta))]
    steps = [int(x[2]) for x in meta]
    return starts, ends, axes, steps


def main():
    path = parse_args()
    info_path = path + "/graph.info"
    data_path = path + "/graph.data"
    outputfile = path + "/model_refactor.onnx"
    with open(info_path, "r") as f:
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
            operators = []
            for line in iter(m.readline, b""):
                if line == b"\n":
                    break
                operators.append(Operator(line))
            graph = Topo(m.readline().strip().strip(b"graph. "))
            _ = m.readline()
            tensors = [Tensor(line) for line in iter(m.readline, b"")]

    with open(data_path, "r") as f:
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
            nodes = []
            # for t in tensors:
            #     if t.size != 0:
            #         print(f"tensor_name is {t.name}")
            initializer = [
                # (
                #     ,
                #     print(f"tensor_name is {t.name}"),
                # )
                make_tensor(
                    t.name,
                    t.dt,
                    t.shape,
                    vals=m[t.offset : t.offset + t.size],
                    raw=True,
                )
                for t in tensors
                if t.size != 0
            ]
            for o in operators:
                node, init = o.to_node(tensors)
                nodes.append(node)
                initializer.extend(init)
            graph = make_graph(
                nodes,
                "graph",
                [
                    make_tensor_value_info(t.name, t.dt, t.shape)
                    for t in (tensors[i] for i in graph.inputs)
                ],
                [
                    make_tensor_value_info(t.name, t.dt, t.shape)
                    for t in (tensors[i] for i in graph.outputs)
                ],
                initializer,
                value_info=[
                    make_tensor_value_info(t.name, t.dt, t.shape)
                    for t in tensors
                    if t.size == 0
                    and t.name not in graph.inputs
                    and t.name not in graph.outputs
                ],
            )
            # model = make_model(
            #     graph, opset_imports=[make_opsetid(domain="", version=13)]
            # )
            model = make_model(
                graph,
                opset_imports=[
                    make_opsetid(domain="refactor", version=1),
                    make_opsetid(domain="", version=13),
                ],
            )
            save_model(
                model,
                outputfile,
                # save_as_external_data=True,
                all_tensors_to_one_file=True,
            )
            check_model(outputfile)


if __name__ == "__main__":
    main()
