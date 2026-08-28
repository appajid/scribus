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
step("reopening document")
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the saved test document")
check(scribus.getVariable(variable_id) == "Third Edition", "user variable did not survive save/reopen")
check(scribus.getVariable("Edition Label") == "Third Edition", "saved name lookup failed")
check(scribus.getVariable("document-title") == "Dynamic Variables Test", "saved built-in metadata lookup failed")

step("deleting variable")
scribus.deleteVariable(variable_id)
check(scribus.listVariables() == [], "deleteVariable failed")
print("DYNAMIC_VARIABLE_TEST_PASSED", flush=True)
scribus.closeDoc()
