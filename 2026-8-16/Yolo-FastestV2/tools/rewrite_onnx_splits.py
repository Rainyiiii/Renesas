from collections import defaultdict
from pathlib import Path
import argparse

import onnx
from onnx import helper, shape_inference


def _shape_map(model):
    inferred = shape_inference.infer_shapes(model)
    values = list(inferred.graph.value_info) + list(inferred.graph.output)
    return {
        value.name: [dimension.dim_value for dimension in value.type.tensor_type.shape.dim]
        for value in values
    }


def rewrite_shuffle_slices(path: Path) -> int:
    model = onnx.load(str(path))
    shapes = _shape_map(model)
    groups = defaultdict(list)
    for index, node in enumerate(model.graph.node):
        if node.op_type == "Slice" and "/backbone/stage" in node.name:
            groups[node.input[0]].append((index, node))

    replacements = {}
    remove_indices = set()
    for source, nodes in groups.items():
        if len(nodes) != 2:
            continue
        nodes.sort(key=lambda item: item[0])
        channels = [shapes[node.output[0]][1] for _, node in nodes]
        if not all(channels):
            continue
        first_index = nodes[0][0]
        prefix = nodes[0][1].name.rsplit("/", 1)[0]
        replacements[first_index] = helper.make_node(
            "Split",
            inputs=[source],
            outputs=[node.output[0] for _, node in nodes],
            name=f"{prefix}/Split",
            axis=1,
            split=channels,
        )
        remove_indices.update(index for index, _ in nodes)

    rewritten = []
    for index, node in enumerate(model.graph.node):
        if index in replacements:
            rewritten.append(replacements[index])
        if index not in remove_indices:
            rewritten.append(node)

    # Drop constants that only supplied starts/ends/axes to removed Slice nodes.
    used_inputs = {name for node in rewritten for name in node.input}
    rewritten = [
        node for node in rewritten
        if node.op_type != "Constant" or any(output in used_inputs for output in node.output)
    ]
    del model.graph.node[:]
    model.graph.node.extend(rewritten)
    onnx.checker.check_model(model)
    onnx.save(model, str(path))
    return len(replacements)


def main() -> None:
    parser = argparse.ArgumentParser(description="Rewrite ShuffleNet Slice pairs as fixed ONNX Split nodes.")
    parser.add_argument("model", type=Path)
    args = parser.parse_args()
    print(f"Replaced {rewrite_shuffle_slices(args.model)} slice pairs")


if __name__ == "__main__":
    main()
