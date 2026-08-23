from pathlib import Path
import argparse
import sys

import torch
import torch.nn as nn

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import model.detector
import utils.utils
from rewrite_onnx_splits import rewrite_shuffle_slices


class UInt8InputWrapper(nn.Module):
    """Expose a camera-friendly uint8 input while keeping the trained model float."""

    def __init__(self, model: nn.Module) -> None:
        super().__init__()
        self.model = model

    def forward(self, images):
        return self.model(images.float() / 255.0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Export a fixed-shape opset 11 ONNX for RUHMI AI Navigator.")
    parser.add_argument("--data", default="data/ho.data")
    parser.add_argument("--weights", default="weights/ho-best.pth")
    parser.add_argument("--output", default="weights/ho-ruhmi-op11.onnx")
    parser.add_argument("--raw", action="store_true", help="Export raw head tensors without sigmoid/softmax decode.")
    parser.add_argument("--npu-friendly", action="store_true",
                        help="Use Ethos-U-friendly contiguous channel splits.")
    parser.add_argument("--uint8-input", action="store_true",
                        help="Export uint8 NCHW input and normalize inside the graph.")
    args = parser.parse_args()

    cfg = utils.utils.load_datafile(args.data)
    device = torch.device("cpu")
    net = model.detector.Detector(
        cfg["classes"],
        cfg["anchor_num"],
        load_param=True,
        export_onnx=not args.raw,
        npu_friendly=args.npu_friendly,
    ).to(device)
    net.load_state_dict(torch.load(args.weights, map_location=device))
    net.eval()

    if args.uint8_input:
        net = UInt8InputWrapper(net)
        sample_dtype = torch.uint8
    else:
        sample_dtype = torch.float32
    sample = torch.zeros(1, 3, cfg["height"], cfg["width"], dtype=sample_dtype, device=device)
    output_names = ["head_s22", "head_s11"] if not args.raw else [
        "reg_s22", "obj_s22", "cls_s22", "reg_s11", "obj_s11", "cls_s11"
    ]

    torch.onnx.export(
        net,
        sample,
        args.output,
        export_params=True,
        opset_version=11,
        do_constant_folding=True,
        input_names=["images"],
        output_names=output_names,
        dynamic_axes=None,
        dynamo=False,
    )
    if args.npu_friendly:
        replaced = rewrite_shuffle_slices(Path(args.output))
        print(f"Replaced {replaced} ShuffleNet slice pairs with ONNX Split nodes")
    print(f"Exported {args.output}")


if __name__ == "__main__":
    main()
