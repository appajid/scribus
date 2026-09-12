#!/usr/bin/env python3

"""End-to-end regression test for Object Style management and undo/redo."""

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
document_path = os.path.join(output_dir, "object-style-management.sla")
if os.path.exists(document_path):
    os.remove(document_path)

check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (20, 20, 20, 20),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_1,
        0,
        1,
    ),
    "could not create document",
)

scribus.createObjectStyle(name="Frame Base", fillcolor="Black", linewidth=2.5)
scribus.createObjectStyle(name="Frame Child", parent="Frame Base", cornerradius=8.0)
scribus.createObjectStyle(name="Replacement", fillcolor="White", linewidth=0.75)

base_frame = scribus.createRect(30, 40, 100, 70, "Base Frame")
child_frame = scribus.createRect(150, 40, 100, 70, "Child Frame")
initial_fill = scribus.getFillColor(base_frame)
scribus.setObjectStyle("Frame Base", base_frame)
check(scribus.getObjectStyle(base_frame) == "Frame Base", "base style was not applied")
check(scribus.getFillColor(base_frame) == "Black", "base appearance was not applied")

scribus.undo()
check(scribus.getObjectStyle(base_frame) == "", "undo did not clear the style assignment")
check(scribus.getFillColor(base_frame) == initial_fill, "undo did not restore the original appearance")
scribus.redo()
check(scribus.getObjectStyle(base_frame) == "Frame Base", "redo did not restore the style assignment")
check(scribus.getFillColor(base_frame) == "Black", "redo did not restore the style appearance")

scribus.setObjectStyle("Frame Child", child_frame)
check(scribus.getFillColor(child_frame) == "Black", "child did not inherit the base appearance")

expect_error(
    lambda: scribus.renameObjectStyle("Frame Base", "Replacement"),
    "rename accepted an existing destination name",
)
expect_error(
    lambda: scribus.deleteObjectStyle("Default Object Style"),
    "default Object Style was deleted",
)

scribus.renameObjectStyle("Frame Base", "Renamed Base")
styles = scribus.getObjectStyles()
check("Frame Base" not in styles and "Renamed Base" in styles, "style definition was not renamed")
check(scribus.getObjectStyle(base_frame) == "Renamed Base", "assigned style was not renamed")
check(scribus.getObjectStyle(child_frame) == "Frame Child", "child assignment changed during parent rename")
check(scribus.getFillColor(child_frame) == "Black", "child inheritance broke during parent rename")

scribus.undo()
styles = scribus.getObjectStyles()
check("Frame Base" in styles and "Renamed Base" not in styles, "undo did not restore the old style definition")
check(scribus.getObjectStyle(base_frame) == "Frame Base", "undo left a dangling renamed assignment")
check(scribus.getFillColor(child_frame) == "Black", "undo did not restore child inheritance")
scribus.redo()
check(scribus.getObjectStyle(base_frame) == "Renamed Base", "redo did not restore the renamed assignment")

expect_error(
    lambda: scribus.deleteObjectStyle("Renamed Base", "Missing Replacement"),
    "delete accepted a missing replacement",
)
scribus.deleteObjectStyle("Renamed Base", "Replacement")
styles = scribus.getObjectStyles()
check("Renamed Base" not in styles, "replaced style definition survived deletion")
check(scribus.getObjectStyle(base_frame) == "Replacement", "assigned object did not use replacement")
check(scribus.getFillColor(base_frame) == "White", "replacement appearance was not applied")
check(scribus.getFillColor(child_frame) == "White", "dependent child did not use the replacement parent")

scribus.undo()
check("Renamed Base" in scribus.getObjectStyles(), "undo did not restore the deleted definition")
check(scribus.getObjectStyle(base_frame) == "Renamed Base", "undo did not restore the deleted assignment")
check(scribus.getFillColor(base_frame) == "Black", "undo did not restore deleted-style appearance")
check(scribus.getFillColor(child_frame) == "Black", "undo did not restore the child's old parent")
scribus.redo()
check(scribus.getObjectStyle(base_frame) == "Replacement", "redo did not restore replacement assignment")

scribus.deleteObjectStyle("Replacement")
check("Replacement" not in scribus.getObjectStyles(), "style survived deletion without replacement")
check(scribus.getObjectStyle(base_frame) == "", "deleted assignment was not cleared")
check(scribus.getFillColor(base_frame) == "White", "clearing the assignment changed current appearance")
check(scribus.getFillColor(child_frame) == "None", "dependent child retained a deleted parent appearance")

scribus.undo()
check("Replacement" in scribus.getObjectStyles(), "undo did not restore the replacement definition")
check(scribus.getObjectStyle(base_frame) == "Replacement", "undo did not restore the replacement association")
check(scribus.getFillColor(child_frame) == "White", "undo did not restore dependent inheritance")
scribus.redo()
check(scribus.getObjectStyle(base_frame) == "", "redo did not clear the deleted association")

scribus.saveDocAs(document_path)
scribus.closeDoc()
check(scribus.openDoc(document_path), "could not reopen saved document")
check("Renamed Base" not in scribus.getObjectStyles(), "deleted style returned after reload")
check("Replacement" not in scribus.getObjectStyles(), "replacement returned after reload")
check(scribus.getObjectStyle(base_frame) == "", "cleared association did not persist")
scribus.closeDoc()

print("OBJECT_STYLE_MANAGEMENT_QA_PASSED", flush=True)
