from pathlib import Path
import argparse

import cv2
import numpy as np


def main() -> None:
    parser = argparse.ArgumentParser(description="Build real-image NCHW calibration samples for RUHMI.")
    parser.add_argument("--images", default="calibration_ho")
    parser.add_argument("--output", default="calibration_ra8p1_192")
    parser.add_argument("--size", type=int, default=192)
    parser.add_argument("--count", type=int, default=100)
    parser.add_argument("--float", action="store_true",
                        help="Write normalized float32 instead of camera-native uint8.")
    parser.add_argument("--rgb", action="store_true",
                        help="Convert OpenCV BGR images to RGB before writing samples.")
    args = parser.parse_args()

    image_dir = Path(args.images)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    image_paths = sorted(
        p for p in image_dir.iterdir()
        if p.suffix.lower() in {".jpg", ".jpeg", ".png", ".bmp"}
    )[:args.count]
    if not image_paths:
        raise SystemExit(f"No calibration images found in {image_dir}")

    for index, image_path in enumerate(image_paths):
        image = cv2.imread(str(image_path), cv2.IMREAD_COLOR)
        if image is None:
            continue
        image = cv2.resize(image, (args.size, args.size), interpolation=cv2.INTER_LINEAR)
        if args.rgb:
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        sample = image.transpose(2, 0, 1)[None, ...]
        if args.float:
            sample = sample.astype(np.float32) / 255.0
        np.save(output_dir / f"sample_{index:04d}.npy", sample)

    print(f"Wrote calibration samples to {output_dir.resolve()}")


if __name__ == "__main__":
    main()
