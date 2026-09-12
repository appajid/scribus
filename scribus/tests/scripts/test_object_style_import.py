#!/usr/bin/env python3

"""End-to-end regression test for document-to-document Object Style import."""

import os
import tempfile

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def expect_error(callback, message):
    try:
        callback()
    except Exception:
        return
    raise AssertionError(message)


output_dir = os.environ.get("SCRIBUS_TEST_OUTPUT_DIR", tempfile.gettempdir())
os.makedirs(output_dir, exist_ok=True)
source_path = os.path.join(output_dir, "object-style-import-source.sla")
destination_path = os.path.join(output_dir, "object-style-import-destination.sla")
for path in (source_path, destination_path):
    if os.path.exists(path):
        os.remove(path)

document_args = (
    scribus.PAPER_A4,
    (20, 20, 20, 20),
    scribus.PORTRAIT,
    1,
    scribus.UNIT_POINTS,
    scribus.PAGE_1,
    0,
    1,
)

check(scribus.newDocument(*document_args), "could not create source document")
scribus.defineColorRGB("Brand Orange", 237, 110, 35)
scribus.createCustomLineStyle(
    "Frame Rule",
    [{"Color": "Brand Orange", "Width": 3.0, "Dash": 1, "LineEnd": 0, "LineJoin": 0, "Shade": 100}],
)
scribus.createObjectStyle(
    name="Frame Base",
    fillcolor="Brand Orange",
    linewidth=2.5,
    customlinestyle="Frame Rule",
)
scribus.createObjectStyle(name="Imported Child", parent="Frame Base", cornerradius=9.0)
scribus.createObjectStyle(name="Not Selected", fillcolor="Black")
scribus.saveDocAs(source_path)
scribus.closeDoc()

check(scribus.newDocument(*document_args), "could not create destination document")
scribus.createCustomLineStyle(
    "Frame Rule",
    [{"Color": "Black", "Width": 0.5, "Dash": 1, "LineEnd": 0, "LineJoin": 0, "Shade": 100}],
)
scribus.createObjectStyle(name="Frame Base", fillcolor="None", linewidth=0.5)

expect_error(
    lambda: scribus.importObjectStyles(source_path, ["Missing Style"]),
    "a missing source style was accepted",
)

renamed = scribus.importObjectStyles(source_path, ["Imported Child"], True)
check(renamed.get("Frame Base") == "Frame Base (2)", "parent name clash was not resolved")
check(renamed.get("Imported Child") == "Imported Child", "unexpected child rename")
check("Not Selected" not in scribus.getObjectStyles(), "selective import included an unselected style")
check("Frame Rule (2)" in scribus.getLineStyles(), "dependent line style was not imported")
check("Brand Orange" in scribus.getColorNames(), "dependent color was not imported")

scribus.undo()
check("Frame Base (2)" not in scribus.getObjectStyles(), "undo did not remove imported parent style")
check("Imported Child" not in scribus.getObjectStyles(), "undo did not remove imported child style")
check("Frame Rule (2)" not in scribus.getLineStyles(), "undo did not remove the imported line style")
check("Frame Rule" in scribus.getLineStyles(), "undo removed the pre-existing line style")
check("Brand Orange" not in scribus.getColorNames(), "undo did not remove the imported color")
scribus.redo()
check("Frame Base (2)" in scribus.getObjectStyles(), "redo did not restore imported parent style")
check("Imported Child" in scribus.getObjectStyles(), "redo did not restore imported child style")
check("Frame Rule (2)" in scribus.getLineStyles(), "redo did not restore the imported line style")
check("Brand Orange" in scribus.getColorNames(), "redo did not restore the imported color")

frame = scribus.createRect(50, 60, 120, 80, "Imported Style Frame")
scribus.setObjectStyle("Imported Child", frame)
check(scribus.getFillColor(frame) == "Brand Orange", "imported parent appearance was not inherited")
check(abs(scribus.getLineWidth(frame) - 2.5) < 0.001, "imported line width was not applied")
check(scribus.getCornerRadius(frame) == 9, "imported child override was not applied")

replaced = scribus.importObjectStyles(source_path, ["Imported Child"], False)
check(replaced.get("Frame Base") == "Frame Base", "replace mode unexpectedly renamed the parent")
check(replaced.get("Imported Child") == "Imported Child", "replace mode unexpectedly renamed the child")

all_styles = scribus.importObjectStyles(source_path)
check("Not Selected" in all_styles, "None did not select every source Object Style")
check("Not Selected" in scribus.getObjectStyles(), "all-style import omitted an Object Style")

scribus.saveDocAs(destination_path)
scribus.closeDoc()
check(scribus.openDoc(destination_path), "could not reopen destination document")
check("Frame Base (2)" in scribus.getObjectStyles(), "renamed imported parent did not persist")
check("Imported Child" in scribus.getObjectStyles(), "imported child did not persist")
scribus.closeDoc()

print("OBJECT_STYLE_IMPORT_QA_PASSED", flush=True)
