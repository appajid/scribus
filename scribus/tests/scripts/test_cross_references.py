#!/usr/bin/env python3

"""
Exercise the first Phase 3 cross-reference vertical slice.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
import json
import os
import shutil
import subprocess
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


def read_sla(path):
    with open(path, "rb") as saved_file:
        data = saved_file.read()
    return gzip.decompress(data) if data.startswith(b"\x1f\x8b") else data


def step(message):
    print("CROSS_REFERENCE_QA: " + message, flush=True)


def preflight_errors():
    report = json.loads(scribus.exportDocumentCheck())
    errors = []
    for page_items in report.get("pages", {}).values():
        errors.extend(item.get("error") for item in page_items)
    for page_items in report.get("masterPages", {}).values():
        errors.extend(item.get("error") for item in page_items)
    return errors


output_path = os.path.join(tempfile.gettempdir(), "scribus_cross_reference_test.sla")
pdf_path = os.path.join(tempfile.gettempdir(), "scribus_cross_reference_test.pdf")
text_path = os.path.join(tempfile.gettempdir(), "scribus_cross_reference_test.txt")
for path in (output_path, pdf_path, text_path):
    if os.path.exists(path):
        os.remove(path)


def rendered_source_text():
    pdftotext = shutil.which("pdftotext")
    if pdftotext is None:
        return None
    for path in (pdf_path, text_path):
        if os.path.exists(path):
            os.remove(path)
    pdf = scribus.PDFfile()
    pdf.file = pdf_path
    pdf.pages = [1]
    pdf.save()
    subprocess.run([pdftotext, "-layout", pdf_path, text_path], check=True)
    with open(text_path, "r", encoding="utf-8") as text_file:
        return text_file.read()

step("creating source and destination frames")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_1,
        0,
        3,
    ),
    "could not create the cross-reference test document",
)
source_frame = scribus.createText(40, 40, 300, 60, "ReferenceSource")
scribus.setText("See page ", source_frame)
second_source_frame = scribus.createText(40, 120, 300, 60, "SecondReferenceSource")
scribus.setText("Also see page ", second_source_frame)
paragraph_source_frame = scribus.createText(40, 200, 300, 60, "ParagraphReferenceSource")
scribus.setText("Read ", paragraph_source_frame)
scribus.gotoPage(2)
target_frame = scribus.createText(40, 120, 300, 60, "ReferenceTarget")
scribus.setText("Chapter Two", target_frame)

step("creating a named target and dynamic page reference")
target_name = scribus.createCrossReferenceTarget("chapter-two", target_frame, 0)
check(target_name == "chapter-two", "target creation returned the wrong name")
check(scribus.listCrossReferenceTargets() == ["chapter-two"], "target list is incorrect")
check(scribus.getCrossReferencePage(target_name) == "2", "target page did not resolve")
reference_label = scribus.insertCrossReference(target_name, source_frame, -1, "Chapter Two page")
check(reference_label == "Chapter Two page", "page-reference insertion returned the wrong label")
second_reference_label = scribus.insertCrossReference(
    target_name, second_source_frame, -1, "Second Chapter Two page"
)
check(second_reference_label == "Second Chapter Two page", "second page-reference insertion failed")
paragraph_reference_label = scribus.insertCrossReference(
    target_name,
    paragraph_source_frame,
    -1,
    "Chapter Two title",
    "paragraph",
    "Chapter: ",
    ".",
)
check(paragraph_reference_label == "Chapter Two title", "paragraph-reference insertion failed")
check(scribus.getCrossReferenceText(target_name) == "Chapter Two", "target paragraph text did not resolve")
rendered_text = rendered_source_text()
if rendered_text is not None:
    check("See page 2" in rendered_text, "inserted page reference did not render its target page")
    check("Read Chapter: Chapter Two." in rendered_text, "paragraph reference did not render with its prefix and suffix")

step("updating paragraph references after target text edits")
scribus.insertText("Revised ", 1, target_frame)
check(
    scribus.getCrossReferenceText(target_name) == "Revised Chapter Two",
    "edited target paragraph text did not resolve",
)
rendered_text = rendered_source_text()
if rendered_text is not None:
    check(
        "Read Chapter: Revised Chapter Two." in rendered_text,
        "paragraph reference did not update after target text changed",
    )

step("rejecting invalid target and reference inputs")
expect_error(
    lambda: scribus.createCrossReferenceTarget("chapter-two", target_frame),
    "a duplicate target name was accepted",
)
expect_error(
    lambda: scribus.createCrossReferenceTarget("", target_frame),
    "an empty target name was accepted",
)
expect_error(
    lambda: scribus.createCrossReferenceTarget("out-of-range", target_frame, 999),
    "an out-of-range target position was accepted",
)
expect_error(
    lambda: scribus.insertCrossReference("missing-target", source_frame),
    "a reference to a missing target was accepted",
)
expect_error(
    lambda: scribus.insertCrossReference(target_name, source_frame, -1, "Invalid", "unknown"),
    "an unsupported cross-reference format was accepted",
)
shape_name = scribus.createRect(360, 220, 100, 50, "NotText")
expect_error(
    lambda: scribus.createCrossReferenceTarget("shape-target", shape_name),
    "a target was inserted into a non-text frame",
)
expect_error(
    lambda: scribus.insertCrossReference(target_name, shape_name),
    "a page reference was inserted into a non-text frame",
)

step("renaming a target without breaking its page references")
scribus.renameCrossReferenceTarget(target_name, "chapter-renamed")
target_name = "chapter-renamed"
check(scribus.listCrossReferenceTargets() == [target_name], "renamed target list is incorrect")
check(scribus.getCrossReferencePage(target_name) == "2", "renamed target lost its page")
check("BrokenCrossReference" not in preflight_errors(), "renaming broke an existing page reference")
rendered_text = rendered_source_text()
if rendered_text is not None:
    check("See page 2" in rendered_text, "page reference did not render after target rename")
expect_error(
    lambda: scribus.getCrossReferencePage("chapter-two"),
    "the old target name remained addressable after rename",
)
expect_error(
    lambda: scribus.renameCrossReferenceTarget(target_name, ""),
    "an empty target name was accepted during rename",
)

step("checking repagination updates")
scribus.newPage(2)
check(scribus.getCrossReferencePage(target_name) == "3", "target did not follow page insertion")
rendered_text = rendered_source_text()
if rendered_text is not None:
    check("See page 3" in rendered_text, "rendered page reference did not update after insertion")
scribus.deletePage(2)
check(scribus.getCrossReferencePage(target_name) == "2", "target did not follow page deletion")

step("checking SLA persistence and reopen")
scribus.saveDocAs(output_path)
saved_data = read_sla(output_path)
check(b'label="chapter-renamed" type="0"' in saved_data, "renamed target was not serialized")
check(b'label="Chapter Two page" type="2"' in saved_data, "page reference was not serialized")
check(
    saved_data.count(b'MARKlabel="chapter-renamed"') == 3,
    "not every page-reference destination followed the target rename",
)
check(b'xrefFormat="1"' in saved_data, "paragraph-reference format was not serialized")
check(b'xrefPrefix="Chapter: "' in saved_data, "cross-reference prefix was not serialized")
check(b'xrefSuffix="."' in saved_data, "cross-reference suffix was not serialized")
check(b'MARKlabel="chapter-two"' not in saved_data, "a stale destination survived target rename")
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the cross-reference document")
check(scribus.listCrossReferenceTargets() == [target_name], "renamed target did not survive reopen")
check(scribus.getCrossReferencePage(target_name) == "2", "target page did not survive reopen")
check(
    scribus.getCrossReferenceText(target_name) == "Revised Chapter Two",
    "target paragraph text did not survive reopen",
)
check("BrokenCrossReference" not in preflight_errors(), "valid page reference failed Preflight")
rendered_text = rendered_source_text()
if rendered_text is not None:
    check(
        "Read Chapter: Revised Chapter Two." in rendered_text,
        "reopened paragraph reference lost its format or affixes",
    )

step("preserving and reporting an externally broken reference")
scribus.closeDoc()
saved_data = read_sla(output_path)
saved_data = saved_data.replace(b'MARKlabel="chapter-renamed"', b'MARKlabel="missing-target"', 1)
with open(output_path, "wb") as saved_file:
    saved_file.write(saved_data)
check(scribus.openDoc(output_path), "could not reopen the malformed reference fixture")
check("BrokenCrossReference" in preflight_errors(), "broken reference was not reported by Preflight")
scribus.saveDoc()
check(
    b'MARKlabel="missing-target"' in read_sla(output_path),
    "saving silently deleted or rewrote the broken reference",
)

step("deleting a used target without deleting its page-reference fields")
expect_error(
    lambda: scribus.deleteCrossReferenceTarget("does-not-exist"),
    "deleting a missing target did not report an error",
)
scribus.deleteCrossReferenceTarget(target_name)
check(scribus.listCrossReferenceTargets() == [], "deleted target remains in the target list")
check("BrokenCrossReference" in preflight_errors(), "target deletion did not flag dependent references")
scribus.saveDoc()
saved_data = read_sla(output_path)
check(
    b'label="chapter-renamed" type="0"' not in saved_data,
    "deleted target was still serialized",
)
check(
    b'MARKlabel="chapter-renamed"' in saved_data,
    "target deletion silently removed a dependent page-reference field",
)
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the document after target deletion")
check("BrokenCrossReference" in preflight_errors(), "broken reference did not survive reopen")

print("CROSS_REFERENCE_QA_PASSED", flush=True)
