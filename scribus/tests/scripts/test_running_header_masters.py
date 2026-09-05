#!/usr/bin/env python3

"""
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

"""Exercise running headers in a realistic facing-page/master-page document."""

import os
import shutil
import subprocess
import tempfile

import scribus


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def step(message):
    print("RUNNING_HEADER_MASTERS_QA: " + message, flush=True)


output_dir = tempfile.gettempdir()
document_path = os.path.join(output_dir, "scribus_running_header_masters.sla")
pdf_path = os.path.join(output_dir, "scribus_running_header_masters.pdf")
text_path = os.path.join(output_dir, "scribus_running_header_masters.txt")
for path in (document_path, pdf_path, text_path):
    if os.path.exists(path):
        os.remove(path)

pdftotext = shutil.which("pdftotext")
check(pdftotext is not None, "pdftotext is required for master-page rendering verification")


def export_pages(pages):
    for path in (pdf_path, text_path):
        if os.path.exists(path):
            os.remove(path)
    pdf = scribus.PDFfile()
    pdf.file = pdf_path
    pdf.pages = pages
    pdf.save()
    subprocess.run([pdftotext, "-layout", pdf_path, text_path], check=True)
    with open(text_path, "r", encoding="utf-8") as text_file:
        return text_file.read().split("\f")[: len(pages)]


def create_heading(page, name, value, y=180):
    scribus.gotoPage(page)
    frame = scribus.createText(55, y, 400, 40, name)
    scribus.setText(value, frame)
    scribus.setParagraphStyle("ChapterTitle", frame)
    return frame


def create_master(name, label, variable_id):
    scribus.createMasterPage(name)
    scribus.editMasterPage(name)
    frame = scribus.createText(50, 35, 480, 35, name + "Frame")
    scribus.setFont("Arial Regular", frame)
    scribus.setText(label, frame)
    scribus.insertVariable(variable_id, frame)
    scribus.closeMasterPage()


step("creating an eight-page facing document")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_2,
        0,
        8,
    ),
    "could not create the facing-page document",
)
scribus.createParagraphStyle("ChapterTitle")
heading_one = create_heading(1, "HeadingOne", "ALPHA CHAPTER")
heading_three = create_heading(3, "HeadingThree", "BRAVO CHAPTER")
create_heading(5, "HeadingFiveFirst", "CHARLIE CHAPTER", 150)
create_heading(5, "HeadingFiveLast", "DELTA CHAPTER", 240)

recent_id = scribus.createRunningHeaderVariable(
    "Current Chapter", "ChapterTitle", "most-recent"
)
first_id = scribus.createRunningHeaderVariable(
    "First Chapter On Page", "ChapterTitle", "first-on-page"
)
last_id = scribus.createRunningHeaderVariable(
    "Last Chapter On Page", "ChapterTitle", "last-on-page"
)

step("creating separate left and right master pages")
create_master("Left Running Header", "LEFT HEADER: ", recent_id)
create_master("Right Running Header", "RIGHT HEADER: ", recent_id)
create_master("First On Page Header", "FIRST HEADER: ", first_id)
create_master("Last On Page Header", "LAST HEADER: ", last_id)
for page in range(1, 9):
    master = "Left Running Header" if page % 2 == 0 else "Right Running Header"
    scribus.applyMasterPage(master, page)
    check(scribus.getMasterPage(page) == master, "master assignment was not retained")

scribus.saveDocAs(document_path)

step("checking page-specific master rendering")
pages = export_pages(list(range(1, 9)))
expected = (
    "ALPHA CHAPTER",
    "ALPHA CHAPTER",
    "BRAVO CHAPTER",
    "BRAVO CHAPTER",
    "DELTA CHAPTER",
    "DELTA CHAPTER",
    "DELTA CHAPTER",
    "DELTA CHAPTER",
)
for page_number, (page_text, expected_heading) in enumerate(zip(pages, expected), 1):
    side_label = "LEFT HEADER: " if page_number % 2 == 0 else "RIGHT HEADER: "
    check(
        side_label + expected_heading in page_text,
        "page {} resolved the wrong master running header".format(page_number),
    )

step("checking first/last modes on a page with two headings")
scribus.applyMasterPage("First On Page Header", 5)
page_five = export_pages([5])[0]
check("FIRST HEADER: CHARLIE CHAPTER" in page_five, "first-on-page master chose the wrong heading")
check("DELTA CHAPTER" in page_five, "page-five source content was not rendered")
scribus.applyMasterPage("Last On Page Header", 5)
page_five = export_pages([5])[0]
check("LAST HEADER: DELTA CHAPTER" in page_five, "last-on-page master chose the wrong heading")

step("checking master reassignment and restoration")
scribus.applyMasterPage("First On Page Header", 4)
page_four = export_pages([4])[0]
check("FIRST HEADER:" in page_four, "reassigned master was not rendered")
check("BRAVO CHAPTER" not in page_four, "page-local header incorrectly carried a prior heading")
scribus.applyMasterPage("Left Running Header", 4)
page_four = export_pages([4])[0]
check("LEFT HEADER: BRAVO CHAPTER" in page_four, "restored master retained stale content")

step("checking source-edit invalidation across master pages")
scribus.setText("BRAVO CHAPTER REVISED", heading_three)
scribus.setParagraphStyle("ChapterTitle", heading_three)
pages = export_pages([3, 4])
check("RIGHT HEADER: BRAVO CHAPTER REVISED" in pages[0], "source page header was stale")
check("LEFT HEADER: BRAVO CHAPTER REVISED" in pages[1], "facing page header was stale")

step("checking insertion and deletion before a running-header source")
scribus.newPage(2, "Left Running Header")
check(scribus.pageCount() == 9, "page insertion failed")
pages = export_pages([2, 4, 5])
check("LEFT HEADER: ALPHA CHAPTER" in pages[0], "inserted page did not carry the prior heading")
check(
    scribus.getMasterPage(4) == "Right Running Header",
    "the shifted source page did not retain its assigned master",
)
check(
    scribus.getMasterPage(5) == "Left Running Header",
    "the shifted continuation page did not retain its assigned master",
)
check("RIGHT HEADER: BRAVO CHAPTER REVISED" in pages[1], "shifted source page resolved incorrectly")
check("LEFT HEADER: BRAVO CHAPTER REVISED" in pages[2], "shifted continuation page resolved incorrectly")
scribus.deletePage(2)
check(scribus.pageCount() == 8, "page deletion failed")
pages = export_pages([3, 4])
check("RIGHT HEADER: BRAVO CHAPTER REVISED" in pages[0], "header was stale after page deletion")
check("LEFT HEADER: BRAVO CHAPTER REVISED" in pages[1], "facing header was stale after page deletion")

step("checking save/reopen persistence")
scribus.saveDoc()
scribus.closeDoc()
check(scribus.openDoc(document_path), "could not reopen the facing-page document")
pages = export_pages([3, 4, 5])
check("RIGHT HEADER: BRAVO CHAPTER REVISED" in pages[0], "reopened source header was incorrect")
check("LEFT HEADER: BRAVO CHAPTER REVISED" in pages[1], "reopened facing header was incorrect")
check("LAST HEADER: DELTA CHAPTER" in pages[2], "reopened reassigned master was incorrect")

print("RUNNING_HEADER_MASTERS_QA_PASSED", flush=True)
scribus.closeDoc()
