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
    2,
)
check(created, "could not create the test document")
scribus.setInfo("Test Author", "Dynamic Variables Test", "Phase 1 regression test")
scribus.createParagraphStyle("ChapterTitle")

step("testing variable CRUD")
variable_id = scribus.createVariable("Edition", "Second Edition")
check(variable_id, "createVariable did not return a stable ID")
check(scribus.getVariable(variable_id) == "Second Edition", "ID lookup failed")
check(scribus.getVariable("Edition") == "Second Edition", "name lookup failed")
check(scribus.getVariable("document-title") == "Dynamic Variables Test", "built-in title did not resolve")
check(scribus.getVariable("page-count") == "2", "built-in page count did not resolve")

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
future_mode_id = "running-header-future-mode"
running_header_xml = (
    b'<Variable id="running-header-test" type="running-header" name="Chapter Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="most-recent"/>'
    b'<Variable id="running-header-future-mode" type="running-header" name="Future Header" value="" '
    b'paragraphStyle="ChapterTitle" mode="from-spread"/>'
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
check(scribus.insertVariable(running_header_id, frame_name) == running_header_id, "running-header insertion failed")

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
scribus.deleteVariable(future_mode_id)
check(scribus.listVariables() == [], "deleteVariable failed")
print("DYNAMIC_VARIABLE_TEST_PASSED", flush=True)
scribus.closeDoc()
