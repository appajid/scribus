#!/usr/bin/env python3

"""
Exercise the first Phase 3 cross-reference vertical slice.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
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
rendered_text = rendered_source_text()
if rendered_text is not None:
    check("See page 2" in rendered_text, "inserted page reference did not render its target page")

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
shape_name = scribus.createRect(40, 220, 100, 50, "NotText")
expect_error(
    lambda: scribus.createCrossReferenceTarget("shape-target", shape_name),
    "a target was inserted into a non-text frame",
)
expect_error(
    lambda: scribus.insertCrossReference(target_name, shape_name),
    "a page reference was inserted into a non-text frame",
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
check(b'label="chapter-two" type="0"' in saved_data, "target mark was not serialized")
check(b'label="Chapter Two page" type="2"' in saved_data, "page reference was not serialized")
check(b'MARKlabel="chapter-two"' in saved_data, "page-reference destination was not serialized")
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the cross-reference document")
check(scribus.listCrossReferenceTargets() == ["chapter-two"], "target did not survive reopen")
check(scribus.getCrossReferencePage(target_name) == "2", "target page did not survive reopen")

print("CROSS_REFERENCE_QA_PASSED", flush=True)
