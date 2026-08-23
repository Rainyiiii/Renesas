from pathlib import Path
import sys
import zipfile

from lxml import etree


for raw_path in sys.argv[1:]:
    path = Path(raw_path)
    with zipfile.ZipFile(path) as archive:
        bad = archive.testzip()
        xml_parts = [
            name
            for name in archive.namelist()
            if name.endswith((".xml", ".rels"))
        ]
        for name in xml_parts:
            etree.fromstring(archive.read(name))
    print(
        path.name,
        "zip_ok" if bad is None else f"bad_entry={bad}",
        f"xml_parts={len(xml_parts)}",
        f"bytes={path.stat().st_size}",
    )
