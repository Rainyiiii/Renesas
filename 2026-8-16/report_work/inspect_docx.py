from __future__ import annotations

import argparse
import json
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET


W = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
NS = {"w": W}


def qn(local: str) -> str:
    return f"{{{W}}}{local}"


def all_text(node: ET.Element) -> str:
    pieces: list[str] = []
    for item in node.iter():
        if item.tag == qn("t") or item.tag.endswith("}t"):
            pieces.append(item.text or "")
        elif item.tag == qn("tab"):
            pieces.append("\t")
        elif item.tag in {qn("br"), qn("cr")}:
            pieces.append("\n")
    return "".join(pieces)


def paragraph_style(paragraph: ET.Element) -> str:
    style = paragraph.find("./w:pPr/w:pStyle", NS)
    return style.attrib.get(qn("val"), "") if style is not None else ""


def paragraph_comments(paragraph: ET.Element) -> dict[str, str]:
    active: list[str] = []
    anchors: dict[str, list[str]] = {}
    for node in paragraph.iter():
        if node.tag == qn("commentRangeStart"):
            cid = node.attrib.get(qn("id"), "")
            if cid:
                active.append(cid)
                anchors.setdefault(cid, [])
        elif node.tag == qn("commentRangeEnd"):
            cid = node.attrib.get(qn("id"), "")
            if cid in active:
                active.remove(cid)
        elif node.tag == qn("t"):
            for cid in active:
                anchors.setdefault(cid, []).append(node.text or "")
        elif node.tag == qn("tab"):
            for cid in active:
                anchors.setdefault(cid, []).append("\t")
    return {cid: "".join(text) for cid, text in anchors.items()}


def parse_comments(archive: zipfile.ZipFile) -> dict[str, dict[str, str]]:
    try:
        root = ET.fromstring(archive.read("word/comments.xml"))
    except KeyError:
        return {}
    comments: dict[str, dict[str, str]] = {}
    for comment in root.findall("w:comment", NS):
        cid = comment.attrib.get(qn("id"), "")
        comments[cid] = {
            "author": comment.attrib.get(qn("author"), ""),
            "date": comment.attrib.get(qn("date"), ""),
            "text": all_text(comment).strip(),
        }
    return comments


def inspect(path: Path) -> dict:
    with zipfile.ZipFile(path) as archive:
        document = ET.fromstring(archive.read("word/document.xml"))
        comments = parse_comments(archive)
        body = document.find("w:body", NS)
        items: list[dict] = []
        comment_occurrences: list[dict] = []
        if body is not None:
            for child in body:
                if child.tag == qn("p"):
                    text = all_text(child).strip()
                    anchors = paragraph_comments(child)
                    item = {
                        "type": "paragraph",
                        "style": paragraph_style(child),
                        "text": text,
                        "comment_anchors": anchors,
                    }
                    items.append(item)
                    for cid, anchor in anchors.items():
                        comment_occurrences.append(
                            {
                                "id": cid,
                                "anchor": anchor,
                                "paragraph": text,
                                **comments.get(cid, {}),
                            }
                        )
                elif child.tag == qn("tbl"):
                    rows: list[list[dict]] = []
                    for row in child.findall("./w:tr", NS):
                        cells: list[dict] = []
                        for cell in row.findall("./w:tc", NS):
                            paras = cell.findall("./w:p", NS)
                            text = "\n".join(
                                piece for piece in (all_text(p).strip() for p in paras) if piece
                            )
                            anchors: dict[str, str] = {}
                            for p in paras:
                                anchors.update(paragraph_comments(p))
                            cells.append({"text": text, "comment_anchors": anchors})
                            for cid, anchor in anchors.items():
                                comment_occurrences.append(
                                    {
                                        "id": cid,
                                        "anchor": anchor,
                                        "paragraph": text,
                                        **comments.get(cid, {}),
                                    }
                                )
                        rows.append(cells)
                    items.append({"type": "table", "rows": rows})
        media = sorted(name for name in archive.namelist() if name.startswith("word/media/"))
        return {
            "path": str(path),
            "items": items,
            "comments": comments,
            "comment_occurrences": comment_occurrences,
            "media": media,
        }


def render_text(data: dict) -> str:
    lines: list[str] = [f"FILE: {data['path']}", ""]
    for index, item in enumerate(data["items"], start=1):
        if item["type"] == "paragraph":
            text = item["text"]
            if text:
                style = f" style={item['style']!r}" if item["style"] else ""
                lines.append(f"P{index:03d}{style}: {text}")
                for cid, anchor in item["comment_anchors"].items():
                    comment = data["comments"].get(cid, {})
                    lines.append(
                        f"  COMMENT {cid} anchor={anchor!r} "
                        f"author={comment.get('author', '')!r}: {comment.get('text', '')}"
                    )
        else:
            lines.append(f"TABLE {index:03d}:")
            for row_index, row in enumerate(item["rows"], start=1):
                values = [cell["text"].replace("\n", " / ") for cell in row]
                lines.append(f"  R{row_index:02d}: " + " || ".join(values))
                for cell_index, cell in enumerate(row, start=1):
                    for cid, anchor in cell["comment_anchors"].items():
                        comment = data["comments"].get(cid, {})
                        lines.append(
                            f"    COMMENT {cid} cell={cell_index} anchor={anchor!r} "
                            f"author={comment.get('author', '')!r}: {comment.get('text', '')}"
                        )
    lines.extend(["", "COMMENTS:"])
    for occurrence in data["comment_occurrences"]:
        lines.append(
            f"- #{occurrence['id']} [{occurrence.get('author', '')}] "
            f"anchor={occurrence['anchor']!r}; comment={occurrence.get('text', '')}; "
            f"context={occurrence['paragraph']}"
        )
    lines.extend(["", f"MEDIA ({len(data['media'])}):"])
    lines.extend(f"- {name}" for name in data["media"])
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--text", type=Path)
    args = parser.parse_args()
    data = inspect(args.input)
    if args.json:
        args.json.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    if args.text:
        args.text.write_text(render_text(data), encoding="utf-8")
    if not args.json and not args.text:
        print(render_text(data))


if __name__ == "__main__":
    main()
