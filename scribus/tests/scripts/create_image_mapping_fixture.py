#!/usr/bin/env python3
"""Create an isolated document for interactive folder-mapping regression tests.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import json
import os
from pathlib import Path
import struct
import shutil
import tempfile
import zlib

import scribus


def write_png(path, color):
    path.parent.mkdir(parents=True, exist_ok=True)

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(b"IHDR", struct.pack(">IIBBBBB", 32, 32, 8, 2, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress((b"\0" + bytes(color) * 32) * 32))
    data += chunk(b"IEND", b"")
    path.write_bytes(data)


if scribus.haveDoc():
    raise RuntimeError("Close the current document before generating a test fixture.")

# Every invocation gets a fresh directory; never overwrite an existing fixture.
root = Path(tempfile.mkdtemp(prefix="image-folder-mapping-",
                            dir=os.environ.get("SCRIBUS_TEST_OUTPUT_DIR")))
old = root / "old-assets"
new = root / "new-assets"
specs = [
    ("Print", old / "print/logo.png", (215, 45, 45)),
    ("PrintCopy", old / "print/logo.png", (215, 45, 45)),
    ("Web", old / "web/logo.png", (45, 90, 215)),
    ("MissingReplacement", old / "missing.png", (120, 120, 120)),
    ("CorruptReplacement", old / "corrupt.png", (230, 170, 40)),
    ("Unrelated", root / "elsewhere/logo.png", (160, 55, 180)),
    ("Available", root / "available.png", (40, 175, 80)),
]
for _, path, color in specs:
    write_png(path, color)

assert scribus.newDocument(scribus.PAPER_A4, (36, 36, 36, 36),
                           scribus.PORTRAIT, 1, scribus.UNIT_POINTS,
                           scribus.PAGE_1, 0, 1)
expected = {}
for index, (name, path, _) in enumerate(specs):
    x = 42 + (index % 3) * 175
    y = 55 + (index // 3) * 210
    label = scribus.createText(x, y, 165, 30, name + "Label")
    scribus.setText(name, label)
    frame = scribus.createImage(x, y + 35, 135, 135, name)
    scribus.loadImage(str(path), frame)
    scribus.setImageScale(5, 5, frame)
    scribus.setImageOffset(1.25, 2.5, frame)
    expected[name] = dict(path=str(path), scale=scribus.getImageScale(frame),
                          offset=scribus.getImageOffset(frame))

# Keep one intentionally empty frame so the Asset Manager's Empty Frames
# filter can be exercised alongside available and missing linked images.
empty_frame = scribus.createImage(392, 510, 135, 135, "Empty")

document = root / "mapping.sla"
scribus.saveDocAs(str(document))
shutil.copyfile(document, root / "baseline.sla")
scribus.closeDoc()
old.rename(new)
(new / "missing.png").rename(root / "missing-held.png")
(new / "corrupt.png").write_bytes(b"Not an image")
(root / "elsewhere").rename(root / "elsewhere-held")
(root / "expected.json").write_text(json.dumps(expected, indent=2), encoding="utf-8")
print("IMAGE_MAPPING_FIXTURE=" + str(root), flush=True)
