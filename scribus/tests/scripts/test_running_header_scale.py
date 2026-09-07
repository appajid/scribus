#!/usr/bin/env python3

"""
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
import os
import tempfile
import time

import scribus


page_total = 500
heading_interval = 5
path = os.path.join(tempfile.gettempdir(), "scribus_running_header_scale.sla")
if os.path.exists(path):
    os.remove(path)

scribus.newDocument(
    scribus.PAPER_A4,
    (20, 20, 20, 20),
    scribus.PORTRAIT,
    1,
    scribus.UNIT_POINTS,
    scribus.PAGE_2,
    scribus.FIRSTPAGELEFT,
    page_total,
)
scribus.createParagraphStyle("ScaleHeading")
scribus.createVariable("Scale Seed", "seed")
contexts = []
for page in range(1, page_total + 1):
    scribus.gotoPage(page)
    contexts.append(scribus.createText(40, 40, 250, 30, "ScaleContext{:03d}".format(page)))
    if (page - 1) % heading_interval == 0:
        source = scribus.createText(40, 100, 250, 30, "ScaleHeading{:03d}".format(page))
        scribus.setText("HEADING {:03d}".format(page), source)
        scribus.setParagraphStyle("ScaleHeading", source)

scribus.saveDocAs(path)
scribus.closeDoc()
with open(path, "rb") as source_file:
    data = source_file.read()
compressed = data.startswith(b"\x1f\x8b")
if compressed:
    data = gzip.decompress(data)
definition = (
    b'<Variable id="scale-recent" type="running-header" name="Scale Recent" value="" '
    b'paragraphStyle="ScaleHeading" mode="most-recent"/>'
    b'<Variable id="scale-spread-first" type="running-header" name="Scale Spread First" value="" '
    b'paragraphStyle="ScaleHeading" mode="first-on-spread"/>'
    b'<Variable id="scale-section-fallback" type="running-header" name="Scale Section Fallback" value="" '
    b'paragraphStyle="ScaleHeading" mode="first-on-page" fallback="section"/>'
)
data = data.replace(b"</DynamicVariables>", definition + b"</DynamicVariables>", 1)
with open(path, "wb") as target_file:
    target_file.write(gzip.compress(data) if compressed else data)

scribus.openDoc(path)
failures = []
cold_start = time.perf_counter()
for page, context in enumerate(contexts, 1):
    source_page = 1 + ((page - 1) // heading_interval) * heading_interval
    expected = "HEADING {:03d}".format(source_page)
    actual = scribus.getVariable("scale-recent", context)
    if actual != expected:
        failures.append(("most-recent", page, actual, expected))
    actual_section = scribus.getVariable("scale-section-fallback", context)
    if actual_section != expected:
        failures.append(("section-fallback", page, actual_section, expected))
    spread_start = page if page % 2 else page - 1
    spread_heading = next(
        (
            candidate
            for candidate in range(spread_start, min(page_total, spread_start + 1) + 1)
            if (candidate - 1) % heading_interval == 0
        ),
        None,
    )
    expected_spread = "" if spread_heading is None else "HEADING {:03d}".format(spread_heading)
    actual_spread = scribus.getVariable("scale-spread-first", context)
    if actual_spread != expected_spread:
        failures.append(("first-on-spread", page, actual_spread, expected_spread))
cold_seconds = time.perf_counter() - cold_start

warm_start = time.perf_counter()
for context in contexts:
    scribus.getVariable("scale-recent", context)
    scribus.getVariable("scale-spread-first", context)
    scribus.getVariable("scale-section-fallback", context)
warm_seconds = time.perf_counter() - warm_start

print("SCALE_QA|pages={}|headings={}|failures={}|cold_seconds={:.6f}|warm_seconds={:.6f}".format(
    page_total, page_total // heading_interval, len(failures), cold_seconds, warm_seconds
), flush=True)
for failure in failures[:10]:
    print("SCALE_QA_FAILURE|{}|{}|{!r}|{!r}".format(*failure), flush=True)
if failures:
    raise AssertionError("{} running headers resolved incorrectly".format(len(failures)))
if cold_seconds > 5.0:
    raise AssertionError("cold running-header resolution exceeded 5 seconds")
if warm_seconds > 1.0:
    raise AssertionError("warm running-header resolution exceeded 1 second")
scribus.closeDoc()
print("RUNNING_HEADER_SCALE_QA_PASSED", flush=True)
