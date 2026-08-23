# Yolo-FastestV2 local training setup

This folder is prepared for VS Code training on Windows.

## Dataset layout

Put your images and YOLO label files here:

```text
datasets/custom/
  train/
    000001.jpg
    000001.txt
  val/
    000101.jpg
    000101.txt
```

Each label file uses YOLO format:

```text
class_id center_x center_y width height
```

All coordinates must be normalized from 0 to 1.

## Common commands

```powershell
.\.venv\Scripts\python.exe tools\make_yolo_lists.py --dataset datasets\custom --names "class0,class1"
.\.venv\Scripts\python.exe genanchors.py --traintxt datasets\custom\train.txt --output_dir data --input_width 352 --input_height 352
.\.venv\Scripts\python.exe train.py --data data\custom.data
```

After generating anchors, copy the first line from `data/anchors6.txt` into `anchors=` in `data/custom.data`.
