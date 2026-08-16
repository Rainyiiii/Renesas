from pathlib import Path
import sys

import torch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import model.detector
import utils.datasets
import utils.utils


cfg = utils.utils.load_datafile("data/ho.data")
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

for attempt in range(50):
    dataset = utils.datasets.TensorDataset(cfg["train"], cfg["width"], cfg["height"], imgaug=True)
    loader = torch.utils.data.DataLoader(
        dataset,
        batch_size=16,
        shuffle=True,
        collate_fn=utils.datasets.collate_fn,
        num_workers=0,
        drop_last=True,
    )
    imgs, _ = next(iter(loader))
    net = model.detector.Detector(cfg["classes"], cfg["anchor_num"], False).to(device)
    x = imgs.to(device).float() / 255.0
    preds = net(x)
    if all(torch.isfinite(pred).all().item() for pred in preds):
        print("attempt", attempt, "ok")
        continue

    print("attempt", attempt, "bad input", imgs.min().item(), imgs.max().item(), imgs.float().mean().item())
    net = model.detector.Detector(cfg["classes"], cfg["anchor_num"], False).to(device)

    def make_hook(name):
        def hook(_module, _inputs, output):
            values = output if isinstance(output, (tuple, list)) else [output]
            ok = all(torch.isfinite(value).all().item() for value in values if torch.is_tensor(value))
            if not ok:
                print("first bad", name)
                raise RuntimeError(name)
        return hook

    for name, module in net.named_modules():
        module.register_forward_hook(make_hook(name))

    try:
        net(x)
    except RuntimeError:
        pass
    break
