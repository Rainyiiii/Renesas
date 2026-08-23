from pathlib import Path
import argparse
import sys

import torch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import model.detector
import utils.datasets
import utils.utils


def main() -> None:
    parser = argparse.ArgumentParser(description="Evaluate a Yolo-FastestV2 checkpoint.")
    parser.add_argument("--data", required=True)
    parser.add_argument("--weights", required=True)
    parser.add_argument("--images", help="Optional image-list file; defaults to the configured validation set.")
    parser.add_argument("--npu-friendly", action="store_true")
    args = parser.parse_args()

    cfg = utils.utils.load_datafile(args.data)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    image_list = args.images or cfg["val"]
    dataset = utils.datasets.TensorDataset(image_list, cfg["width"], cfg["height"], imgaug=False)
    loader = torch.utils.data.DataLoader(
        dataset,
        batch_size=cfg["batch_size"],
        shuffle=False,
        collate_fn=utils.datasets.collate_fn,
        num_workers=0,
    )
    net = model.detector.Detector(
        cfg["classes"], cfg["anchor_num"], load_param=True,
        npu_friendly=args.npu_friendly,
    ).to(device)
    net.load_state_dict(torch.load(args.weights, map_location=device))
    net.eval()

    precision, recall, ap, f1 = utils.utils.evaluation(loader, cfg, net, device, 0.3)
    print(f"Precision: {precision:.6f}")
    print(f"Recall:    {recall:.6f}")
    print(f"AP:        {ap:.6f}")
    print(f"F1:        {f1:.6f}")


if __name__ == "__main__":
    main()
