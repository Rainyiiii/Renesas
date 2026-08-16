from __future__ import annotations

import argparse
from pathlib import Path

import pypdfium2 as pdfium
from PIL import Image, ImageDraw


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("pdf", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--columns", type=int, default=4)
    args = parser.parse_args()

    args.output.mkdir(parents=True, exist_ok=True)
    document = pdfium.PdfDocument(args.pdf)
    rendered: list[Image.Image] = []
    for page_index in range(len(document)):
        page = document[page_index]
        bitmap = page.render(scale=args.scale)
        image = bitmap.to_pil().convert("RGB")
        path = args.output / f"page-{page_index + 1:02d}.png"
        image.save(path)
        rendered.append(image)

    thumb_width = 330
    margin = 18
    label_height = 28
    thumbs: list[Image.Image] = []
    for index, image in enumerate(rendered, start=1):
        height = round(image.height * thumb_width / image.width)
        thumb = image.resize((thumb_width, height))
        tile = Image.new("RGB", (thumb_width, height + label_height), "white")
        tile.paste(thumb, (0, label_height))
        ImageDraw.Draw(tile).text((8, 5), f"Page {index}", fill="black")
        thumbs.append(tile)

    columns = max(1, args.columns)
    rows = (len(thumbs) + columns - 1) // columns
    tile_width = thumb_width + margin
    tile_height = max(image.height for image in thumbs) + margin
    sheet = Image.new("RGB", (columns * tile_width + margin, rows * tile_height + margin), "#dddddd")
    for index, thumb in enumerate(thumbs):
        x = margin + (index % columns) * tile_width
        y = margin + (index // columns) * tile_height
        sheet.paste(thumb, (x, y))
    sheet.save(args.output / "contact-sheet.png")
    print(f"pages={len(rendered)}")


if __name__ == "__main__":
    main()
