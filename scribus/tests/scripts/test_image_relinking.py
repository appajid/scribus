#!/usr/bin/env python3

"""Exercise failure-safe image relinking, undo/redo and SLA persistence.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
import os
import struct
import tempfile
import zlib

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def png_chunk(chunk_type, data):
    return (
        struct.pack(">I", len(data))
        + chunk_type
        + data
        + struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    )


def write_png(path, red, green, blue):
    width = 8
    height = 8
    row = b"\x00" + bytes((red, green, blue)) * width
    data = b"\x89PNG\r\n\x1a\n"
    data += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    data += png_chunk(b"IDAT", zlib.compress(row * height))
    data += png_chunk(b"IEND", b"")
    with open(path, "wb") as image_file:
        image_file.write(data)


def close_enough(actual, expected):
    return abs(actual - expected) < 0.01


def same_pair(actual, expected):
    return close_enough(actual[0], expected[0]) and close_enough(actual[1], expected[1])


def read_sla(path):
    with open(path, "rb") as saved_file:
        data = saved_file.read()
    return gzip.decompress(data) if data.startswith(b"\x1f\x8b") else data


def step(message):
    print("IMAGE_RELINK_QA: " + message, flush=True)


output_dir = os.environ.get("SCRIBUS_TEST_OUTPUT_DIR", tempfile.gettempdir())
os.makedirs(output_dir, exist_ok=True)
original_path = os.path.abspath(os.path.join(output_dir, "relink-original.png"))
replacement_path = os.path.abspath(os.path.join(output_dir, "relink-replacement.png"))
missing_path = os.path.abspath(os.path.join(output_dir, "does-not-exist.png"))
document_path = os.path.abspath(os.path.join(output_dir, "image-relinking.sla"))
for old_path in (original_path, replacement_path, missing_path, document_path):
    if os.path.exists(old_path):
        os.remove(old_path)
write_png(original_path, 220, 45, 45)
write_png(replacement_path, 35, 95, 220)

step("creating an image frame with non-default crop and scale")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_1,
        0,
        1,
    ),
    "could not create the image-relink test document",
)
frame = scribus.createImage(72, 72, 240, 180, "RelinkFrame")
scribus.loadImage(original_path, frame)
scribus.setImageScale(1.25, 0.75, frame)
scribus.setImageOffset(17.0, 23.0, frame)
expected_scale = scribus.getImageScale(frame)
expected_offset = scribus.getImageOffset(frame)

step("relinking successfully and preserving frame-level settings")
check(scribus.relinkImage(replacement_path, frame) is True, "valid replacement was rejected")
check(os.path.abspath(scribus.getImageFile(frame)) == replacement_path, "replacement path was not applied")
check(same_pair(scribus.getImageScale(frame), expected_scale), "image scale changed during relink")
check(same_pair(scribus.getImageOffset(frame), expected_offset), "image crop offset changed during relink")

step("rejecting a missing replacement without damaging the current link")
check(scribus.relinkImage(missing_path, frame) is False, "missing replacement was accepted")
check(os.path.abspath(scribus.getImageFile(frame)) == replacement_path, "failed relink changed the stored path")
check(same_pair(scribus.getImageScale(frame), expected_scale), "failed relink changed image scale")
check(same_pair(scribus.getImageOffset(frame), expected_offset), "failed relink changed crop offset")

step("undoing and redoing the successful relink")
scribus.undo()
check(os.path.abspath(scribus.getImageFile(frame)) == original_path, "undo did not restore the original link")
check(same_pair(scribus.getImageScale(frame), expected_scale), "undo changed image scale")
check(same_pair(scribus.getImageOffset(frame), expected_offset), "undo changed crop offset")
scribus.redo()
check(os.path.abspath(scribus.getImageFile(frame)) == replacement_path, "redo did not restore the replacement link")
check(same_pair(scribus.getImageScale(frame), expected_scale), "redo changed image scale")
check(same_pair(scribus.getImageOffset(frame), expected_offset), "redo changed crop offset")

step("saving and reopening the relinked document")
scribus.saveDocAs(document_path)
saved_data = read_sla(document_path)
check(b"relink-replacement.png" in saved_data, "replacement path was not written to the SLA")
scribus.closeDoc()
check(scribus.openDoc(document_path), "could not reopen the relinked document")
check(os.path.abspath(scribus.getImageFile(frame)) == replacement_path, "replacement link did not survive reopen")
check(same_pair(scribus.getImageScale(frame), expected_scale), "image scale did not survive reopen")
check(same_pair(scribus.getImageOffset(frame), expected_offset), "crop offset did not survive reopen")

print("IMAGE_RELINK_QA_PASSED", flush=True)
