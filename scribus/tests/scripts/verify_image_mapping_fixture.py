#!/usr/bin/env python3
"""Verify a saved GUI mapping fixture after cancellation, apply, undo or redo.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import json
import gzip
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import scribus

root = Path(os.environ["SCRIBUS_MAPPING_FIXTURE"])
state = os.environ["SCRIBUS_MAPPING_STATE"]
assert state in ("original", "relinked"), "Expected original or relinked state"
assert not scribus.haveDoc(), "Run verification in an isolated Scribus process"
expected = json.loads((root / "expected.json").read_text(encoding="utf-8"))


def image_geometry(path):
    data = path.read_bytes()
    if data.startswith(b"\x1f\x8b"):
        data = gzip.decompress(data)
    return {item.get("AutoName"): tuple(float(item.get(key, "0")) for key in
            ("ImageScaleX", "ImageScaleY", "ImageOffsetX", "ImageOffsetY"))
            for item in ET.fromstring(data).iter("PageObject") if item.get("AutoName") in expected}


baseline = image_geometry(root / "baseline.sla")
saved = image_geometry(root / "mapping.sla")
assert set(baseline) == set(saved) == set(expected), "Missing image frames in saved fixture"
assert scribus.openDoc(str(root / "mapping.sla")), "Could not reopen the fixture"
for name, settings in expected.items():
    path = Path(settings["path"])
    if state == "relinked" and name in ("Print", "PrintCopy", "Web"):
        path = root / "new-assets" / path.relative_to(root / "old-assets")
    actual = Path(scribus.getImageFile(name))
    assert actual.resolve() == path.resolve(), f"Wrong link for {name}: {actual} != {path}"
    # The Scripter scale getter includes image DPI, which is unavailable for a
    # missing image. Compare the stored scale and crop, independent of DPI.
    assert all(abs(a - b) < 0.001 for a, b in zip(saved[name], baseline[name])), (
        f"Changed stored scale/crop for {name}: {saved[name]} != {baseline[name]}"
    )
    assert all(abs(a - b) < 0.001 for a, b in zip(scribus.getImageOffset(name), settings["offset"])), (
        f"Changed crop after reopening {name}"
    )
scribus.closeDoc()
print(f"IMAGE_MAPPING_VERIFIED state={state} frames={len(expected)}", flush=True)
