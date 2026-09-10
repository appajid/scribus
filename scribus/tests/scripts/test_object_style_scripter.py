#!/usr/bin/env python3

"""End-to-end regression test for the Object Style Scripter API."""

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


output_path = os.environ.get(
    "SCRIBUS_OBJECT_STYLE_SCRIPT_OUTPUT",
    os.path.join(tempfile.gettempdir(), "object_style_scripter.sla"),
)
if os.path.exists(output_path):
    os.remove(output_path)

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

scribus.createObjectStyle(
    name="Frame Base",
    fillcolor="Black",
    fillshade=83.0,
    linecolor="Black",
    lineshade=71.0,
    linewidth=2.5,
    linestyle=2,
    linecap=32,
    linejoin=128,
    filltransparency=0.2,
    linetransparency=0.4,
    fillblendmode=3,
    lineblendmode=4,
    shortcut="Ctrl+Alt+8",
)
scribus.createObjectStyle(
    name="చిత్ర చట్రం",
    parent="Frame Base",
    cornerradius=12.0,
)
scribus.createObjectStyle(name="Unused Object Style", fillcolor="Black")

styles = scribus.getObjectStyles()
check("Default Object Style" in styles, "default object style is missing")
check("Frame Base" in styles and "చిత్ర చట్రం" in styles, "created styles are missing")
expect_error(
    lambda: scribus.createObjectStyle(name="Frame Base"),
    "duplicate style name was accepted",
)
expect_error(
    lambda: scribus.createObjectStyle(name="Broken", parent="Missing Parent"),
    "missing parent style was accepted",
)

first = scribus.createRect(50, 60, 120, 80, "Styled One")
second = scribus.createRect(220, 60, 120, 80, "Styled Two")
scribus.setObjectStyle("చిత్ర చట్రం", first)
check(scribus.getObjectStyle(first) == "చిత్ర చట్రం", "named application failed")
check(scribus.getFillColor(first) == "Black", "inherited fill was not applied")
check(abs(scribus.getLineWidth(first) - 2.5) < 0.001, "inherited line width was not applied")
check(scribus.getCornerRadius(first) == 12, "child corner radius was not applied")
check(scribus.getSize(first) == (120.0, 80.0), "style application changed geometry")

scribus.deselectAll()
scribus.selectObject(first)
scribus.selectObject(second)
scribus.setObjectStyle("చిత్ర చట్రం")
check(scribus.getObjectStyle(second) == "చిత్ర చట్రం", "selection-wide application failed")

scribus.setObjectStyle("", second)
check(scribus.getObjectStyle(second) == "", "style association was not cleared")
check(scribus.getFillColor(second) == "Black", "clearing a style changed appearance")
check(scribus.getCornerRadius(second) == 12, "clearing a style changed the corner radius")

removed = scribus.removeUnusedStyles()
styles = scribus.getObjectStyles()
check(removed >= 1, "unused-style cleanup did not remove the unused object style")
check("Unused Object Style" not in styles, "unused object style survived cleanup")
check("Frame Base" in styles, "parent of an applied object style was incorrectly removed")
check("చిత్ర చట్రం" in styles, "applied object style was incorrectly removed")

scribus.saveDocAs(output_path)
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen saved document")
check(scribus.getObjectStyle(first) == "చిత్ర చట్రం", "assigned style did not survive reload")
check(scribus.getObjectStyle(second) == "", "cleared style unexpectedly returned after reload")
check(scribus.getSize(first) == (120.0, 80.0), "reload changed styled item geometry")
scribus.closeDoc()

print("OBJECT_STYLE_SCRIPTER_QA_PASSED", flush=True)
