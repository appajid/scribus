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
refresh_path = os.path.join(output_dir, "object_style_refresh.sla")
for path in (source_path, roundtrip_path, refresh_path):
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
scribus.createRect(72, 90, 120, 80, "Styled Frame")
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
item_marker = b"    <PageObject "
check(item_marker in source_data, "could not find the page item insertion point")
source_data = source_data.replace(
    item_marker,
    item_marker + 'ObjectStyle="చిత్ర చట్రం" '.encode("utf-8"),
    1,
)
write_sla(source_path, source_data, source_compressed)

step("opening, duplicating and saving a styled page item through Scribus")
check(scribus.openDoc(source_path), "could not open the object-style SLA fixture")
duplicate_names = scribus.duplicateObjects("Styled Frame")
check(len(duplicate_names) == 1, "styled page item could not be duplicated")
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

page_items = root.findall(".//PageObject")
check(len(page_items) == 2, "styled item copy was not preserved")
expected_item = {
    "ObjectStyle": "చిత్ర చట్రం",
    "Width": "120",
    "Height": "80",
    "FillColor": "Red",
    "FillShade": "87.5",
    "LineColor": "Black",
    "LineShade": "72",
    "LineWidth": "2.25",
    "LinePenStyle": "2",
    "LineCapStyle": "32",
    "LineJoinStyle": "128",
    "FillTransparency": "0.25",
    "LineTransparency": "0.5",
    "FillBlendMode": "3",
    "LineBlendMode": "4",
    "CornerRadius": "12.5",
    "NamedLineStyle": "Default Line Style",
}
for item in page_items:
    for attribute, expected in expected_item.items():
        check(item.attrib.get(attribute) == expected,
              "%s was not applied or copied with the object style" % attribute)

step("refreshing a linked item from a changed object-style definition")
updated_data = roundtrip_data.replace(
    'Name="చిత్ర చట్రం"'.encode("utf-8"),
    'Name="చిత్ర చట్రం" FillColor="Green"'.encode("utf-8"),
    1,
).replace(b'FillColor="Red" CornerRadius="12.5"', b'CornerRadius="15.5"', 1)
write_sla(roundtrip_path, updated_data, False)
check(scribus.openDoc(roundtrip_path), "could not reopen the changed object-style fixture")
scribus.saveDocAs(refresh_path)
scribus.closeDoc()

refresh_data, _ = read_sla(refresh_path)
refresh_root = ET.fromstring(refresh_data)
for item in refresh_root.findall(".//PageObject"):
    check(item.attrib.get("ObjectStyle") == "చిత్ర చట్రం", "linked style reference was lost")
    check(item.attrib.get("FillColor") == "Green", "linked fill did not refresh")
    check(item.attrib.get("CornerRadius") == "15.5", "linked corner radius did not refresh")
    check(item.attrib.get("Width") == "120" and item.attrib.get("Height") == "80",
          "style refresh changed item geometry")

print("OBJECT_STYLE_PERSISTENCE_QA_PASSED", flush=True)
