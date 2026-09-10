#!/usr/bin/env python3

"""
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

"""Exercise first/last running-header resolution across facing-page spreads."""

import gzip
import os
import tempfile

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def step(message):
    print("RUNNING_HEADER_SPREADS_QA: " + message, flush=True)


def create_text(page, name, value="", y=40):
    scribus.gotoPage(page)
    frame = scribus.createText(40, y, 300, 35, name)
    if value:
        scribus.setText(value, frame)
    return frame


def create_heading(page, name, value, y=140):
    frame = create_text(page, name, value, y)
    scribus.setParagraphStyle("SpreadHeading", frame)
    return frame


output_dir = os.environ.get("SCRIBUS_TEST_OUTPUT_DIR", tempfile.gettempdir())
output_path = os.path.join(output_dir, "scribus_running_header_spreads.sla")
if os.path.exists(output_path):
    os.remove(output_path)

step("creating facing pages with an unpaired right-hand cover")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_2,
        scribus.FIRSTPAGERIGHT,
        7,
    ),
    "could not create the facing-page document",
)
scribus.createParagraphStyle("SpreadHeading")

cover_context = create_text(1, "CoverContext")
create_heading(1, "CoverHeading", "COVER")
left_context = create_text(2, "LeftContext")
create_heading(2, "LeftHeading", "ALPHA")
right_context = create_text(3, "RightContext")
right_heading = create_heading(3, "RightHeading", "BETA")
empty_left_context = create_text(4, "EmptyLeftContext")
empty_right_context = create_text(5, "EmptyRightContext")
later_left_context = create_text(6, "LaterLeftContext")
create_heading(6, "LaterLeftHeading", "GAMMA")
later_right_context = create_text(7, "LaterRightContext")
create_heading(7, "LaterRightHeading", "DELTA")

first_id = scribus.createRunningHeaderVariable(
    "First Heading On Spread", "SpreadHeading", "first-on-spread"
)
last_id = scribus.createRunningHeaderVariable(
    "Last Heading On Spread", "SpreadHeading", "last-on-spread"
)

step("checking cover, paired-spread, and empty-spread resolution")
check(scribus.getVariable(first_id, cover_context) == "COVER", "cover first value is wrong")
check(scribus.getVariable(last_id, cover_context) == "COVER", "cover last value is wrong")
for context in (left_context, right_context):
    check(scribus.getVariable(first_id, context) == "ALPHA", "paired spread first value is wrong")
    check(scribus.getVariable(last_id, context) == "BETA", "paired spread last value is wrong")
for context in (empty_left_context, empty_right_context):
    check(scribus.getVariable(first_id, context) == "", "first value crossed an empty spread")
    check(scribus.getVariable(last_id, context) == "", "last value crossed an empty spread")
for context in (later_left_context, later_right_context):
    check(scribus.getVariable(first_id, context) == "GAMMA", "later spread first value is wrong")
    check(scribus.getVariable(last_id, context) == "DELTA", "later spread last value is wrong")

step("checking source-edit invalidation on both sides of a spread")
scribus.setText("BETA REVISED", right_heading)
scribus.setParagraphStyle("SpreadHeading", right_heading)
check(scribus.getVariable(last_id, left_context) == "BETA REVISED", "left context stayed stale")
check(scribus.getVariable(last_id, right_context) == "BETA REVISED", "right context stayed stale")

step("checking spread regrouping after page insertion and deletion")
scribus.newPage(2)
check(scribus.pageCount() == 8, "page insertion failed")
check(scribus.getVariable(first_id, left_context) == "ALPHA", "shifted left first value is wrong")
check(scribus.getVariable(last_id, left_context) == "ALPHA", "shifted left spread was not regrouped")
check(scribus.getVariable(first_id, right_context) == "BETA REVISED", "shifted right spread was not regrouped")
check(scribus.getVariable(last_id, right_context) == "BETA REVISED", "shifted right last value is wrong")
scribus.deletePage(2)
check(scribus.pageCount() == 7, "page deletion failed")
check(scribus.getVariable(first_id, right_context) == "ALPHA", "first value stayed stale after deletion")
check(scribus.getVariable(last_id, left_context) == "BETA REVISED", "last value stayed stale after deletion")

step("checking RTL preserves semantic document reading order")
scribus.setRTL(True)
check(scribus.getVariable(first_id, left_context) == "ALPHA", "RTL changed first document-order value")
check(scribus.getVariable(last_id, right_context) == "BETA REVISED", "RTL changed last document-order value")

step("checking SLA persistence and reopen behavior")
scribus.saveDocAs(output_path)
with open(output_path, "rb") as saved_file:
    saved_data = saved_file.read()
if saved_data.startswith(b"\x1f\x8b"):
    saved_data = gzip.decompress(saved_data)
check(b'mode="first-on-spread"' in saved_data, "first-on-spread was not serialized")
check(b'mode="last-on-spread"' in saved_data, "last-on-spread was not serialized")
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen spread test document")
check(scribus.getVariable(first_id, "LeftContext") == "ALPHA", "reopened first value is wrong")
check(scribus.getVariable(last_id, "RightContext") == "BETA REVISED", "reopened last value is wrong")
scribus.closeDoc()

step("checking single-page documents use the current page as their spread")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_1,
        scribus.FIRSTPAGERIGHT,
        1,
    ),
    "could not create the single-page document",
)
scribus.createParagraphStyle("SpreadHeading")
single_context = create_text(1, "SingleContext")
create_heading(1, "SingleFirst", "ONE", 120)
create_heading(1, "SingleLast", "TWO", 180)
single_first_id = scribus.createRunningHeaderVariable(
    "Single First", "SpreadHeading", "first-on-spread"
)
single_last_id = scribus.createRunningHeaderVariable(
    "Single Last", "SpreadHeading", "last-on-spread"
)
check(scribus.getVariable(single_first_id, single_context) == "ONE", "single-page first value is wrong")
check(scribus.getVariable(single_last_id, single_context) == "TWO", "single-page last value is wrong")
scribus.closeDoc()

print("RUNNING_HEADER_SPREADS_QA_PASSED", flush=True)
