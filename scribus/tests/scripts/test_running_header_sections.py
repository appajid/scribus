#!/usr/bin/env python3

"""
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

"""Exercise section-aware running-header fallback and section boundaries."""

import gzip
import os
import re
import tempfile

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def expect_error(function, message):
    try:
        function()
    except Exception:
        return
    raise AssertionError(message)


def step(message):
    print("RUNNING_HEADER_SECTIONS_QA: " + message, flush=True)


def create_text(page, name, value="", y=40):
    scribus.gotoPage(page)
    frame = scribus.createText(40, y, 320, 35, name)
    if value:
        scribus.setText(value, frame)
    return frame


def create_heading(page, name, value, y=140):
    frame = create_text(page, name, value, y)
    scribus.setParagraphStyle("SectionHeading", frame)
    return frame


output_path = os.path.join(tempfile.gettempdir(), "scribus_running_header_sections.sla")
if os.path.exists(output_path):
    os.remove(output_path)

step("creating a facing document and fallback variables through the Python API")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_2,
        scribus.FIRSTPAGELEFT,
        8,
    ),
    "could not create the section fallback document",
)
scribus.createParagraphStyle("SectionHeading")
create_heading(1, "OpeningHeading", "INTRODUCTION")
early_context = create_text(2, "EarlyContext")
old_boundary_heading = create_heading(5, "OldBoundaryHeading", "OLD SECTION")
old_boundary_context = create_text(5, "OldBoundaryContext")
new_boundary_context = create_text(6, "NewBoundaryContext")
create_heading(7, "NewSectionHeading", "NEW SECTION")
new_section_context = create_text(8, "NewSectionContext")

no_fallback_id = scribus.createRunningHeaderVariable(
    "No Fallback", "SectionHeading", "first-on-page"
)
section_fallback_id = scribus.createRunningHeaderVariable(
    "Section Fallback", "SectionHeading", "first-on-page", "as-entered", False, "section"
)
document_fallback_id = scribus.createRunningHeaderVariable(
    "Document Fallback", "SectionHeading", "first-on-page", "as-entered", False, "document"
)
spread_section_id = scribus.createRunningHeaderVariable(
    "Spread Section Fallback", "SectionHeading", "first-on-spread", "as-entered", False, "section"
)
spread_document_id = scribus.createRunningHeaderVariable(
    "Spread Document Fallback", "SectionHeading", "first-on-spread", "as-entered", False, "document"
)
editable_id = scribus.createRunningHeaderVariable(
    "Editable Section Fallback", "SectionHeading", "first-on-page", "as-entered", False, "section"
)

expect_error(
    lambda: scribus.createRunningHeaderVariable(
        "Invalid Fallback", "SectionHeading", "first-on-page", "as-entered", False, "future"
    ),
    "an unsupported fallback was accepted",
)
expect_error(
    lambda: scribus.createRunningHeaderVariable(
        "Invalid Recent Fallback", "SectionHeading", "most-recent", "as-entered", False, "section"
    ),
    "most-recent accepted an inapplicable fallback",
)
expect_error(
    lambda: scribus.createRunningHeaderVariable(
        "Missing Style", "Style That Does Not Exist", "most-recent"
    ),
    "a running header accepted a missing paragraph style",
)

step("saving and defining two document sections in the SLA fixture")
scribus.saveDocAs(output_path)
scribus.closeDoc()
with open(output_path, "rb") as saved_file:
    saved_data = saved_file.read()
compressed = saved_data.startswith(b"\x1f\x8b")
if compressed:
    saved_data = gzip.decompress(saved_data)
check(b'fallback="section"' in saved_data, "section fallback was not serialized")
check(b'fallback="document"' in saved_data, "document fallback was not serialized")
sections_xml = (
    b'<Sections>'
    b'<Section Number="0" Name="Front Matter" From="0" To="4" Type="Type_1_2_3" '
    b'Start="1" Reversed="0" Active="1" FillChar="0" FieldWidth="0"/>'
    b'<Section Number="1" Name="Body" From="5" To="7" Type="Type_1_2_3" '
    b'Start="1" Reversed="0" Active="1" FillChar="0" FieldWidth="0"/>'
    b'</Sections>'
)
saved_data, replacement_count = re.subn(
    br"<Sections>.*?</Sections>", sections_xml, saved_data, count=1, flags=re.DOTALL
)
check(replacement_count == 1, "could not replace the document sections")
legacy_definition = (
    b'<Variable id="legacy-no-fallback" type="running-header" name="Legacy No Fallback" value="" '
    b'paragraphStyle="SectionHeading" mode="first-on-page" textCase="as-entered" '
    b'removeTrailingPunctuation="0"/>'
    b'<Variable id="unsupported-fallback" type="running-header" name="Unsupported Fallback" value="" '
    b'paragraphStyle="SectionHeading" mode="first-on-page" textCase="as-entered" '
    b'removeTrailingPunctuation="0" fallback="future"/>'
    b'<Variable id="missing-style" type="running-header" name="Missing Style Fixture" value="" '
    b'paragraphStyle="Style That Does Not Exist" mode="most-recent" textCase="as-entered" '
    b'removeTrailingPunctuation="0" fallback="none"/>'
)
saved_data = saved_data.replace(b"</DynamicVariables>", legacy_definition + b"</DynamicVariables>", 1)
with open(output_path, "wb") as target_file:
    target_file.write(gzip.compress(saved_data) if compressed else saved_data)

step("checking section and document fallback at the section boundary")
check(scribus.openDoc(output_path), "could not reopen the section fixture")
check(scribus.getVariable(no_fallback_id, "EarlyContext") == "", "no-fallback carried a heading")
check(
    scribus.getVariable(section_fallback_id, "EarlyContext") == "INTRODUCTION",
    "section fallback missed an earlier heading in the same section",
)
check(
    scribus.getVariable(document_fallback_id, "EarlyContext") == "INTRODUCTION",
    "document fallback missed an earlier heading",
)
check(
    scribus.getVariable(section_fallback_id, "NewBoundaryContext") == "",
    "section fallback crossed into the previous section",
)
check(
    scribus.getVariable(document_fallback_id, "NewBoundaryContext") == "OLD SECTION",
    "document fallback did not cross the section boundary",
)
check(
    scribus.getVariable(spread_section_id, "OldBoundaryContext") == "OLD SECTION",
    "old side of a split spread resolved incorrectly",
)
check(
    scribus.getVariable(spread_section_id, "NewBoundaryContext") == "",
    "section-aware spread included a heading from the adjacent section",
)
check(
    scribus.getVariable(spread_document_id, "NewBoundaryContext") == "OLD SECTION",
    "document-scoped spread did not include its adjacent page",
)
check(
    scribus.getVariable(section_fallback_id, "NewSectionContext") == "NEW SECTION",
    "section fallback missed a heading in the new section",
)
check(
    scribus.getVariable("legacy-no-fallback", "NewSectionContext") == "",
    "a legacy definition without fallback changed behavior",
)
check(
    scribus.getVariable("unsupported-fallback", "NewSectionContext") == "",
    "an unsupported fallback resolved content",
)
check(
    scribus.getVariable("missing-style", "NewSectionContext") == "",
    "a running header with a missing style resolved content",
)

step("checking API updates preserve or explicitly change fallback")
scribus.setRunningHeaderVariable(
    editable_id, "Editable Section Fallback", "SectionHeading", "last-on-page"
)
check(
    scribus.getVariable(editable_id, "NewSectionContext") == "NEW SECTION",
    "an update with omitted options did not preserve section fallback",
)
scribus.setRunningHeaderVariable(
    editable_id,
    "Editable Section Fallback",
    "SectionHeading",
    "last-on-page",
    "as-entered",
    False,
    "none",
)
check(
    scribus.getVariable(editable_id, "NewSectionContext") == "",
    "an explicit no-fallback update was ignored",
)

step("checking edit and deletion invalidation for fallback sources")
scribus.setText("OLD SECTION REVISED", old_boundary_heading)
scribus.setParagraphStyle("SectionHeading", old_boundary_heading)
check(
    scribus.getVariable(document_fallback_id, "NewBoundaryContext") == "OLD SECTION REVISED",
    "document fallback stayed stale after editing its source",
)
scribus.deleteObject(old_boundary_heading)
check(
    scribus.getVariable(document_fallback_id, "NewBoundaryContext") == "INTRODUCTION",
    "document fallback stayed stale after deleting its source",
)
check(
    scribus.getVariable(section_fallback_id, "NewBoundaryContext") == "",
    "section fallback crossed a boundary after source deletion",
)

step("checking round-trip persistence")
scribus.saveDoc()
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the round-tripped document")
check(
    scribus.getVariable(document_fallback_id, "NewBoundaryContext") == "INTRODUCTION",
    "document fallback changed after round trip",
)
check(
    scribus.getVariable(section_fallback_id, "NewBoundaryContext") == "",
    "section boundary changed after round trip",
)
check(
    scribus.getVariable("unsupported-fallback", "NewSectionContext") == "",
    "an unsupported fallback became active after round trip",
)
check(
    scribus.getVariable("missing-style", "NewSectionContext") == "",
    "a missing-style running header became active after round trip",
)
scribus.closeDoc()

print("RUNNING_HEADER_SECTIONS_QA_PASSED", flush=True)
