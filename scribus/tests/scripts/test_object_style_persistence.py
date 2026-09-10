#!/usr/bin/env python3

"""
Exercise document-owned object styles and current-format SLA persistence.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
import os
import tempfile
import xml.etree.ElementTree as ET

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def read_sla(path):
    with open(path, "rb") as saved_file:
        data = saved_file.read()
    return (gzip.decompress(data), True) if data.startswith(b"\x1f\x8b") else (data, False)


def write_sla(path, data, compressed):
    with open(path, "wb") as output_file:
        output_file.write(gzip.compress(data) if compressed else data)


def step(message):
    print("OBJECT_STYLE_QA: " + message, flush=True)


output_dir = os.environ.get("SCRIBUS_TEST_OUTPUT_DIR", tempfile.gettempdir())
os.makedirs(output_dir, exist_ok=True)
source_path = os.path.join(output_dir, "object_style_source.sla")
roundtrip_path = os.path.join(output_dir, "object_style_roundtrip.sla")
for path in (source_path, roundtrip_path):
    if os.path.exists(path):
        os.remove(path)

step("creating a current-format document with its default object style")
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
    "could not create the object-style test document",
)
scribus.saveDocAs(source_path)
scribus.closeDoc()

source_data, source_compressed = read_sla(source_path)
check(source_data.count(b'<ObjectStyle Name="Default Object Style"') == 1,
      "new document did not serialize exactly one default object style")

step("injecting inherited object styles into the SLA fixture")
insertion_point = b"    <TableStyle"
check(insertion_point in source_data, "could not find the style insertion point")
custom_styles = (
    '    <ObjectStyle Name="Brand Frame" Shortcut="Ctrl+Alt+6" '
    'FillColor="Blue" FillShade="87.5" LineColor="Black" LineShade="72" '
    'LineWidth="2.25" LineStyle="2" LineCap="32" LineJoin="128" '
    'FillTransparency="0.25" LineTransparency="0.5" FillBlendMode="3" '
    'LineBlendMode="4" CornerRadius="6.5" CustomLineStyle="Default Line Style"/>\n'
    '    <ObjectStyle Name="చిత్ర చట్రం" Parent="Brand Frame" Shortcut="Ctrl+Alt+7" '
    'FillColor="Red" CornerRadius="12.5"/>\n'
).encode("utf-8")
source_data = source_data.replace(insertion_point, custom_styles + insertion_point, 1)
write_sla(source_path, source_data, source_compressed)

step("opening and saving the fixture through Scribus")
check(scribus.openDoc(source_path), "could not open the object-style SLA fixture")
scribus.saveDocAs(roundtrip_path)
scribus.closeDoc()

roundtrip_data, _ = read_sla(roundtrip_path)
root = ET.fromstring(roundtrip_data)
styles = root.findall(".//ObjectStyle")
styles_by_name = {style.attrib.get("Name"): style.attrib for style in styles}
check("Default Object Style" in styles_by_name, "default object style was lost")
check("Brand Frame" in styles_by_name, "base object style was lost")
check("చిత్ర చట్రం" in styles_by_name, "Unicode child object style was lost")

base = styles_by_name["Brand Frame"]
child = styles_by_name["చిత్ర చట్రం"]
expected_base = {
    "Shortcut": "Ctrl+Alt+6",
    "FillColor": "Blue",
    "FillShade": "87.5",
    "LineColor": "Black",
    "LineShade": "72",
    "LineWidth": "2.25",
    "LineStyle": "2",
    "LineCap": "32",
    "LineJoin": "128",
    "FillTransparency": "0.25",
    "LineTransparency": "0.5",
    "FillBlendMode": "3",
    "LineBlendMode": "4",
    "CornerRadius": "6.5",
    "CustomLineStyle": "Default Line Style",
}
for attribute, expected in expected_base.items():
    check(base.get(attribute) == expected, "%s did not survive round-trip" % attribute)
check(child.get("Parent") == "Brand Frame", "object-style inheritance was lost")
check(child.get("Shortcut") == "Ctrl+Alt+7", "Unicode style shortcut was lost")
check(child.get("FillColor") == "Red", "child fill override was lost")
check(child.get("CornerRadius") == "12.5", "child corner override was lost")
check("LineColor" not in child, "an inherited child property was flattened")

style_names = [style.attrib.get("Name") for style in styles]
check(style_names.index("Brand Frame") < style_names.index("చిత్ర చట్రం"),
      "parent object style was not serialized before its child")

print("OBJECT_STYLE_PERSISTENCE_QA_PASSED", flush=True)
