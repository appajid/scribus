#!/usr/bin/env python3

"""
Regression test for Phase 1 dynamic variables.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import os
import tempfile
import gzip
import shutil
import subprocess

import scribus


def step(message):
    print("DYNAMIC_VARIABLE_TEST: " + message, flush=True)


def check(condition, message):
    if not condition:
        raise AssertionError(message)


output_path = os.environ.get(
    "SCRIBUS_DYNAMIC_VARIABLE_TEST_OUTPUT",
    os.path.join(tempfile.gettempdir(), "scribus_dynamic_variables_test.sla"),
)
if os.path.exists(output_path):
    os.remove(output_path)

step("creating document")
created = scribus.newDocument(
    scribus.PAPER_A4,
    (20, 20, 20, 20),
    scribus.PORTRAIT,
    1,
    scribus.UNIT_POINTS,
    scribus.PAGE_1,
    0,
    3,
)
check(created, "could not create the test document")
scribus.setInfo("Test Author", "Dynamic Variables Test", "Phase 1 regression test")
scribus.createParagraphStyle("ChapterTitle")
scribus.createParagraphStyle("RecursiveHeading")
scribus.createParagraphStyle("FlowHeading")

step("testing variable CRUD")
variable_id = scribus.createVariable("Edition", "Second Edition")
check(variable_id, "createVariable did not return a stable ID")
check(scribus.getVariable(variable_id) == "Second Edition", "ID lookup failed")
check(scribus.getVariable("Edition") == "Second Edition", "name lookup failed")
check(scribus.getVariable("document-title") == "Dynamic Variables Test", "built-in title did not resolve")
check(scribus.getVariable("page-count") == "3", "built-in page count did not resolve")

scribus.setVariable(variable_id, "Third Edition")
scribus.renameVariable(variable_id, "Edition Label")
check(scribus.getVariable("Edition Label") == "Third Edition", "update or rename failed")
check(scribus.listVariables() == [(variable_id, "Edition Label", "Third Edition")], "variable list is incorrect")

step("inserting variables")
frame_name = scribus.createText(40, 40, 300, 80, "DynamicVariableFrame")
scribus.setFont("Arial Regular", frame_name)
check(scribus.insertVariable(variable_id, frame_name) == variable_id, "user variable insertion failed")
title_id = scribus.insertVariable("document-title", frame_name)
check(title_id == "builtin:document-title", "built-in insertion returned the wrong ID")
check(scribus.getVariable("current-page", frame_name) == "1", "contextual current-page lookup failed")

step("creating styled running-header sources")
lower_heading = scribus.createText(40, 220, 300, 40, "LowerChapterHeading")
scribus.setText("Last Visual Heading", lower_heading)
scribus.setParagraphStyle("ChapterTitle", lower_heading)
upper_heading = scribus.createText(40, 140, 300, 40, "UpperChapterHeading")
scribus.setText("First Visual Heading", upper_heading)
scribus.setParagraphStyle("ChapterTitle", upper_heading)
scribus.gotoPage(2)
second_page_context = scribus.createText(40, 40, 300, 40, "SecondPageContext")
second_page_heading = scribus.createText(40, 140, 300, 40, "SecondPageHeading")
scribus.setText("Second Page Heading", second_page_heading)
scribus.setParagraphStyle("ChapterTitle", second_page_heading)
scribus.gotoPage(3)
third_page_context = scribus.createText(40, 40, 300, 40, "ThirdPageContext")
scribus.gotoPage(1)
flow_heading_text = "LINKED HEADING " * 20
flow_start = scribus.createText(360, 140, 120, 50, "FlowHeadingStart")
scribus.setText(flow_heading_text, flow_start)
scribus.setParagraphStyle("FlowHeading", flow_start)
scribus.gotoPage(2)
flow_end = scribus.createText(360, 140, 120, 200, "FlowHeadingEnd")
scribus.linkTextFrames(flow_start, flow_end)
flow_local = scribus.createText(360, 400, 180, 80, "FlowLocalHeading")
scribus.setText("LOCAL HEADING", flow_local)
scribus.setParagraphStyle("FlowHeading", flow_local)
scribus.gotoPage(1)

step("saving document")
scribus.saveDocAs(output_path)
with open(output_path, "rb") as saved_file:
    saved_data = saved_file.read()
if saved_data.startswith(b"\x1f\x8b"):
    saved_data = gzip.decompress(saved_data)
check(b"<DynamicVariables" in saved_data, "dynamic variable definitions were not serialized")
check(variable_id.encode("utf-8") in saved_data, "the stable variable ID was not serialized")
check(b'variableId="builtin:document-title"' in saved_data, "the built-in mark reference was not serialized")

step("injecting running-header definitions")
scribus.closeDoc()
running_header_id = "running-header-test"
first_on_page_id = "running-header-first-on-page"
last_on_page_id = "running-header-last-on-page"
future_mode_id = "running-header-future-mode"
recursive_header_id = "running-header-recursion-guard"
flow_first_id = "running-header-flow-first"
flow_recent_id = "running-header-flow-recent"
running_header_xml = (
    b'<Variable id="running-header-test" type="running-header" name="Chapter Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="most-recent"/>'
    b'<Variable id="running-header-first-on-page" type="running-header" name="First Chapter Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="first-on-page"/>'
    b'<Variable id="running-header-last-on-page" type="running-header" name="Last Chapter Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="last-on-page"/>'
    b'<Variable id="running-header-future-mode" type="running-header" name="Future Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="from-spread"/>'
    b'<Variable id="running-header-recursion-guard" type="running-header" name="Recursive Header" value="" '
    b'paragraphStyle="RecursiveHeading" mode="first-on-page"/>'
    b'<Variable id="running-header-flow-first" type="running-header" name="Flow First" value="" '
    b'paragraphStyle="FlowHeading" mode="first-on-page"/>'
    b'<Variable id="running-header-flow-recent" type="running-header" name="Flow Recent" value="" '
    b'paragraphStyle="FlowHeading" mode="most-recent"/>'
)
check(b"</DynamicVariables>" in saved_data, "dynamic variable container is incomplete")
saved_data = saved_data.replace(b"</DynamicVariables>", running_header_xml + b"</DynamicVariables>", 1)
with open(output_path, "wb") as saved_file:
    saved_file.write(saved_data)

step("reopening document")
check(scribus.openDoc(output_path), "could not reopen the saved test document")
check(scribus.getVariable(variable_id) == "Third Edition", "user variable did not survive save/reopen")
check(scribus.getVariable("Edition Label") == "Third Edition", "saved name lookup failed")
check(scribus.getVariable("document-title") == "Dynamic Variables Test", "saved built-in metadata lookup failed")
check(scribus.getVariable(running_header_id) == "", "unresolved running header did not fail safely")
check(scribus.getVariable(future_mode_id) == "", "unknown running-header mode did not fail safely")
check(
    scribus.getVariable(running_header_id, frame_name) == "Last Visual Heading",
    "most-recent did not use the final matching paragraph on its page",
)
check(
    scribus.getVariable(running_header_id, third_page_context) == "Second Page Heading",
    "most-recent did not carry a heading forward to a later page",
)
scribus.setText("Updated Second Page Heading", second_page_heading)
scribus.setParagraphStyle("ChapterTitle", second_page_heading)
check(
    scribus.getVariable(running_header_id, third_page_context) == "Updated Second Page Heading",
    "most-recent cache was not invalidated after editing its source heading",
)
check(
    scribus.getVariable(first_on_page_id, frame_name) == "First Visual Heading",
    "first-on-page did not use visual page order",
)
scribus.setParagraphStyle("ChapterTitle", lower_heading)
check(
    scribus.getVariable(first_on_page_id, upper_heading) == "Last Visual Heading",
    "first-on-page did not exclude its first source context",
)
check(
    scribus.getVariable(first_on_page_id, lower_heading) == "First Visual Heading",
    "first-on-page reused a value cached for a different context frame",
)
check(
    scribus.getVariable(last_on_page_id, frame_name) == "Last Visual Heading",
    "last-on-page did not use visual page order",
)
check(
    scribus.getVariable(first_on_page_id, second_page_context) == "Updated Second Page Heading",
    "first-on-page did not isolate candidates to the context page",
)
check(scribus.insertVariable(first_on_page_id, frame_name) == first_on_page_id, "first-on-page insertion failed")
scribus.setText("Updated First Heading", upper_heading)
scribus.setParagraphStyle("ChapterTitle", upper_heading)
check(
    scribus.getVariable(first_on_page_id, frame_name) == "Updated First Heading",
    "first-on-page did not update after source text changed",
)
scribus.moveObjectAbs(40, 300, upper_heading)
check(
    scribus.getVariable(first_on_page_id, frame_name) == "Last Visual Heading",
    "first-on-page did not update after source frame moved",
)
check(
    scribus.getVariable(last_on_page_id, frame_name) == "Updated First Heading",
    "last-on-page did not update after source frame moved",
)
check(scribus.insertVariable(running_header_id, frame_name) == running_header_id, "running-header insertion failed")

step("excluding linked paragraph continuations from page-local headers")
check(
    scribus.getVariable(flow_first_id, second_page_context) == "LOCAL HEADING",
    "first-on-page did not use the page-local heading",
)
scribus.deleteObject(flow_local)
check(
    scribus.getVariable(flow_first_id, second_page_context) == "",
    "first-on-page treated a linked paragraph continuation as a new heading",
)
check(
    scribus.getVariable(flow_recent_id, second_page_context) == flow_heading_text.strip(),
    "most-recent did not carry the linked heading from its starting page",
)

step("testing a running header on an applied master page")
master_page_name = "Running Header Master"
scribus.createMasterPage(master_page_name)
scribus.editMasterPage(master_page_name)
master_header = scribus.createText(40, 760, 300, 40, "MasterRunningHeader")
scribus.setFont("Arial Regular", master_header)
check(
    scribus.insertVariable(running_header_id, master_header) == running_header_id,
    "could not insert a running header on the master page",
)
scribus.closeMasterPage()
for page_number in (1, 2, 3):
    scribus.applyMasterPage(master_page_name, page_number)

pdf_path = os.path.splitext(output_path)[0] + "-master.pdf"
pdf_text_path = os.path.splitext(output_path)[0] + "-master.txt"
for generated_path in (pdf_path, pdf_text_path):
    if os.path.exists(generated_path):
        os.remove(generated_path)

def export_master_pdf(pages):
    pdf = scribus.PDFfile()
    pdf.file = pdf_path
    pdf.pages = pages
    pdf.save()

def extract_pdf_pages():
    pdftotext = shutil.which("pdftotext")
    if not pdftotext:
        return None
    subprocess.run([pdftotext, "-layout", pdf_path, pdf_text_path], check=True)
    with open(pdf_text_path, "r", encoding="utf-8") as text_file:
        return text_file.read().split("\f")

export_master_pdf([1, 2, 3])
pdf_pages = extract_pdf_pages()
if pdf_pages is not None:
    check("Updated First Heading" in pdf_pages[0], "master header used the wrong page-one context")
    check("Updated Second Page Heading" in pdf_pages[1], "master header used the wrong page-two context")
    check("Updated Second Page Heading" in pdf_pages[2], "master header did not carry forward on page three")

scribus.setText("Final Second Page Heading", second_page_heading)
scribus.setParagraphStyle("ChapterTitle", second_page_heading)
export_master_pdf([3])
pdf_pages = extract_pdf_pages()
if pdf_pages is not None:
    check("Final Second Page Heading" in pdf_pages[0], "master header was stale after editing its source")

step("invalidating running headers after deleting a source frame")
scribus.deleteObject(second_page_heading)
check(
    scribus.getVariable(running_header_id, third_page_context) == "Updated First Heading",
    "most-recent cache was not invalidated after deleting its source heading",
)
export_master_pdf([3])
pdf_pages = extract_pdf_pages()
if pdf_pages is not None:
    check("Updated First Heading" in pdf_pages[0], "master header was stale after deleting its source")
    check("Final Second Page Heading" not in pdf_pages[0], "deleted heading remained in the master header")

step("checking cyclic running-header layout safety")
scribus.gotoPage(3)
recursive_first = scribus.createText(360, 620, 180, 40, "RecursiveHeaderFirst")
recursive_second = scribus.createText(360, 680, 180, 40, "RecursiveHeaderSecond")
scribus.insertVariable(recursive_header_id, recursive_first)
scribus.setParagraphStyle("RecursiveHeading", recursive_first)
scribus.insertVariable(recursive_header_id, recursive_second)
scribus.setParagraphStyle("RecursiveHeading", recursive_second)
recursive_value = scribus.getVariable(recursive_header_id, third_page_context)
check(recursive_value == "", "cyclic running-header resolution did not fail safely")

step("round-tripping running-header definitions")
scribus.saveDoc()
with open(output_path, "rb") as saved_file:
    round_trip_data = saved_file.read()
if round_trip_data.startswith(b"\x1f\x8b"):
    round_trip_data = gzip.decompress(round_trip_data)
check(b'type="running-header"' in round_trip_data, "running-header type was not preserved")
check(b'paragraphStyle="ChapterTitle"' in round_trip_data, "running-header style was not preserved")
check(b'mode="most-recent"' in round_trip_data, "running-header mode was not preserved")
check(b'mode="from-spread"' in round_trip_data, "unknown future mode was not preserved")
check(b'variableId="running-header-test"' in round_trip_data, "running-header mark reference was not serialized")

step("deleting variable")
scribus.deleteVariable(variable_id)
scribus.deleteVariable(running_header_id)
scribus.deleteVariable(first_on_page_id)
scribus.deleteVariable(last_on_page_id)
scribus.deleteVariable(future_mode_id)
scribus.deleteVariable(recursive_header_id)
scribus.deleteVariable(flow_first_id)
scribus.deleteVariable(flow_recent_id)
check(scribus.listVariables() == [], "deleteVariable failed")
print("DYNAMIC_VARIABLE_TEST_PASSED", flush=True)
scribus.closeDoc()
