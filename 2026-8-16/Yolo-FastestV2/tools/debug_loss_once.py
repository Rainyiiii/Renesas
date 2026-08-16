import torch
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import model.detector
import utils.datasets
import utils.loss
import utils.utils


cfg = utils.utils.load_datafile("data/ho.data")
dataset = utils.datasets.TensorDataset(cfg["train"], cfg["width"], cfg["height"], imgaug=True)
loader = torch.utils.data.DataLoader(
    dataset,
    batch_size=16,
    shuffle=True,
    collate_fn=utils.datasets.collate_fn,
    num_workers=0,
    drop_last=True,
)

imgs, targets = next(iter(loader))
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
net = model.detector.Detector(cfg["classes"], cfg["anchor_num"], False).to(device)
preds = net(imgs.to(device).float() / 255.0)
targets = targets.to(device)

print("targets finite", torch.isfinite(targets).all().item(), targets[:, 1:].min().item(), targets[:, 1:].max().item())
tcls, tbox, indices, anchors = utils.loss.build_target(preds, targets, cfg, device)
for idx, box in enumerate(tbox):
    print("tbox", idx, box.shape, torch.isfinite(box).all().item(), box.min().item(), box.max().item())
for idx, anchor in enumerate(anchors):
    print("anchors", idx, anchor.shape, torch.isfinite(anchor).all().item(), anchor.min().item(), anchor.max().item())

for i, pred in enumerate(preds):
    if i % 3 != 0:
        continue
    layer = utils.loss.layer_index[i]
    pred = pred.reshape(pred.shape[0], cfg["anchor_num"], -1, pred.shape[2], pred.shape[3])
    pred = pred.permute(0, 1, 3, 4, 2)
    b, a, gj, gi = indices[layer]
    ps = pred[b, a, gj, gi]
    pxy = ps[:, :2].sigmoid() * 2. - 0.5
    pwh = (ps[:, 2:4].sigmoid() * 2) ** 2 * anchors[layer]
    pbox = torch.cat((pxy, pwh), 1)
    ciou = utils.loss.bbox_iou(pbox.t(), tbox[layer], x1y1x2y2=False, CIoU=True)
    print("layer", layer, "pbox finite", torch.isfinite(pbox).all().item(), "pbox min/max", pbox.min().item(), pbox.max().item())
    print("layer", layer, "ciou finite", torch.isfinite(ciou).all().item(), "nan count", torch.isnan(ciou).sum().item())

losses = utils.loss.compute_loss(preds, targets, cfg, device)
print("losses", [float(x.detach().cpu()) for x in losses])
