import argparse
from pathlib import Path


IMAGE_EXTS = {".bmp", ".jpg", ".jpeg", ".png"}


def image_files(folder: Path) -> list[Path]:
    return sorted(
        path for path in folder.rglob("*")
        if path.is_file() and path.suffix.lower() in IMAGE_EXTS
    )


def write_list(images: list[Path], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="\n") as handle:
        for image in images:
            handle.write(f"{image.resolve().as_posix()}\n")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate Yolo-FastestV2 train.txt and val.txt from train/val folders."
    )
    parser.add_argument("--dataset", default="datasets/custom", help="Dataset root containing train and val folders.")
    parser.add_argument("--names", default="", help="Optional comma-separated class names.")
    args = parser.parse_args()

    root = Path(args.dataset)
    train_dir = root / "train"
    val_dir = root / "val"

    if not train_dir.exists():
        raise SystemExit(f"Missing train folder: {train_dir}")
    if not val_dir.exists():
        raise SystemExit(f"Missing val folder: {val_dir}")

    train_images = image_files(train_dir)
    val_images = image_files(val_dir)
    if not train_images:
        raise SystemExit(f"No images found in {train_dir}")
    if not val_images:
        raise SystemExit(f"No images found in {val_dir}")

    missing_labels = [
        image for image in train_images + val_images
        if not image.with_suffix(".txt").exists()
    ]
    if missing_labels:
        preview = "\n".join(str(path) for path in missing_labels[:10])
        raise SystemExit(f"Missing label files for images:\n{preview}")

    write_list(train_images, root / "train.txt")
    write_list(val_images, root / "val.txt")

    test_dir = root / "test"
    if test_dir.exists():
        test_images = image_files(test_dir)
        test_missing_labels = [image for image in test_images if not image.with_suffix(".txt").exists()]
        if test_missing_labels:
            preview = "\n".join(str(path) for path in test_missing_labels[:10])
            raise SystemExit(f"Missing label files for test images:\n{preview}")
        if test_images:
            write_list(test_images, root / "test.txt")
            print(f"Wrote {root / 'test.txt'} ({len(test_images)} images)")

    if args.names:
        names = [name.strip() for name in args.names.split(",") if name.strip()]
        Path("data/custom.names").write_text("\n".join(names) + "\n", encoding="utf-8")
        data_file = Path("data/custom.data")
        text = data_file.read_text(encoding="utf-8")
        text = text.replace("classes=1", f"classes={len(names)}")
        data_file.write_text(text, encoding="utf-8")

    print(f"Wrote {root / 'train.txt'} ({len(train_images)} images)")
    print(f"Wrote {root / 'val.txt'} ({len(val_images)} images)")


if __name__ == "__main__":
    main()
