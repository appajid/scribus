#!/usr/bin/env python3

"""
Exercise anchored images and tables, reflow, validation and SLA persistence.

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
"""

import gzip
import os
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
    print("ANCHORED_OBJECT_QA: " + message, flush=True)


output_path = os.path.join(tempfile.gettempdir(), "scribus_anchored_object_test.sla")
if os.path.exists(output_path):
    os.remove(output_path)

step("creating a story, image frame and table")
check(
    scribus.newDocument(
        scribus.PAPER_A4,
        (36, 36, 36, 36),
        scribus.PORTRAIT,
        1,
        scribus.UNIT_POINTS,
        scribus.PAGE_1,
        0,
        1,
    ),
    "could not create the anchored-object test document",
)
story = scribus.createText(50, 50, 480, 700, "AnchoredStory")
paragraphs = [
    "Paragraph %02d contains enough words to form a stable line of body text." % index
    for index in range(1, 26)
]
scribus.setText("\n".join(paragraphs), story)
image = scribus.createImage(80, 80, 96, 72, "AnchoredImage")
table = scribus.createTable(80, 180, 180, 70, 2, 3, "AnchoredTable")

step("inserting both object types into specific story positions")
image_id = scribus.insertAnchoredObject(image, story, 260)
table_id = scribus.insertAnchoredObject(table, story, 620)
check(isinstance(image_id, int) and image_id >= 0, "image anchor did not return an identifier")
check(isinstance(table_id, int) and table_id >= 0, "table anchor did not return an identifier")
check(image_id != table_id, "anchored objects received duplicate identifiers")

step("configuring custom image positioning and table above-line positioning")
image_options = {
    "mode": scribus.ANCHOR_MODE_CUSTOM,
    "horizontalReference": scribus.ANCHOR_HREF_COLUMN,
    "verticalReference": scribus.ANCHOR_VREF_LINE,
    "horizontalAlignment": scribus.ANCHOR_HALIGN_RIGHT,
    "verticalAlignment": scribus.ANCHOR_VALIGN_BASELINE,
    "wrapMode": scribus.ANCHOR_WRAP_BOUNDING_BOX,
    "xOffset": -8.0,
    "yOffset": 3.0,
    "wrapLeft": 6.0,
    "wrapTop": 4.0,
    "wrapRight": 6.0,
    "wrapBottom": 4.0,
    "keepWithinBounds": True,
    "preventManualPositioning": True,
}
scribus.setAnchoredObjectOptions(image, image_options)
scribus.setAnchoredObjectOptions(
    table,
    {
        "mode": scribus.ANCHOR_MODE_ABOVE_LINE,
        "horizontalReference": scribus.ANCHOR_HREF_COLUMN,
        "horizontalAlignment": scribus.ANCHOR_HALIGN_CENTER,
        "xOffset": 0.0,
        "yOffset": 5.0,
    },
)
second_image_id = scribus.insertAnchoredObject(image, story, 1000)
check(second_image_id == image_id, "reusing an inline object changed its identifier")
stored_image_options = scribus.getAnchoredObjectOptions(image)
for key, expected in image_options.items():
    actual = stored_image_options[key]
    if isinstance(expected, float):
        check(abs(actual - expected) < 0.01, "image option %s was not applied" % key)
    else:
        check(actual == expected, "image option %s was not applied" % key)
check(
    scribus.getAnchoredObjectOptions(table)["mode"] == scribus.ANCHOR_MODE_ABOVE_LINE,
    "table did not enter above-line mode",
)

step("checking that inserting text above moves both anchors with the story")
image_rect_before = scribus.getAnchoredObjectRect(image, story)
image_rects_before = scribus.getAnchoredObjectRects(image, story)
table_rect_before = scribus.getAnchoredObjectRect(table, story)
check(len(image_rects_before) == 2, "two occurrences of the image did not receive independent anchors")
scribus.insertText(
    "Inserted heading line one.\nInserted heading line two.\nInserted heading line three.\n",
    0,
    story,
)
image_rect_after = scribus.getAnchoredObjectRect(image, story)
image_rects_after = scribus.getAnchoredObjectRects(image, story)
table_rect_after = scribus.getAnchoredObjectRect(table, story)
step("image occurrences before=%r after=%r" % (image_rects_before, image_rects_after))
check(len(image_rects_after) == 2, "an image occurrence was lost during reflow")
check(image_rect_after[1] > image_rect_before[1] + 10.0, "image did not move after story reflow")
check(
    abs(image_rects_after[1][1] - image_rects_before[1][1]) > 10.0,
    "second image occurrence did not move after reflow",
)
check(table_rect_after[1] > table_rect_before[1] + 10.0, "table did not move after story reflow")
check(abs(image_rect_after[2] - 96.0) < 2.0, "image anchor width changed during reflow")
check(
    scribus.getAnchoredObjectRects(image, story) == image_rects_after,
    "repeated layout changed anchored-object geometry",
)

step("rejecting malformed options and invalid destinations")
expect_error(
    lambda: scribus.setAnchoredObjectOptions(image, {"unknownOption": 1}),
    "an unknown option was accepted",
)
expect_error(
    lambda: scribus.setAnchoredObjectOptions(image, {"mode": 99}),
    "an invalid anchor mode was accepted",
)
expect_error(
    lambda: scribus.setAnchoredObjectOptions(image, {"wrapLeft": -1.0}),
    "a negative wrap offset was accepted",
)
shape = scribus.createRect(400, 760, 20, 20, "NotATextFrame")
expect_error(
    lambda: scribus.insertAnchoredObject(shape, image, 0),
    "an inline image was accepted as a text destination",
)

step("checking current-format SLA persistence and reopen")
scribus.saveDocAs(output_path)
saved_data = read_sla(output_path)
check(b'ANCHORMODE="2"' in saved_data or b'AnchorMode="2"' in saved_data, "custom mode was not serialized")
check(b'ANCHORWRAP="1"' in saved_data or b'AnchorWrapMode="1"' in saved_data, "wrap mode was not serialized")
check(b'ANCHORKEEP="1"' in saved_data or b'AnchorKeepWithinBounds="1"' in saved_data, "bounds option was not serialized")
image_marker = ('Object="%d"' % image_id).encode("ascii")
check(saved_data.count(image_marker) == 2, "both image anchor markers were not serialized")
scribus.closeDoc()
check(scribus.openDoc(output_path), "could not reopen the anchored-object document")
reopened_options = scribus.getAnchoredObjectOptions(image)
check(reopened_options["mode"] == scribus.ANCHOR_MODE_CUSTOM, "custom mode did not survive reopen")
check(
    reopened_options["wrapMode"] == scribus.ANCHOR_WRAP_BOUNDING_BOX,
    "wrap mode did not survive reopen",
)
check(reopened_options["keepWithinBounds"] is True, "bounds option did not survive reopen")
reopened_rect = scribus.getAnchoredObjectRect(image, story)
check(len(scribus.getAnchoredObjectRects(image, story)) == 2, "both image occurrences did not survive reopen")
step("resolved image rectangle before reopen=%r after reopen=%r" % (image_rect_after, reopened_rect))
check(abs(reopened_rect[1] - image_rect_after[1]) < 2.0, "resolved image position changed after reopen")

print("ANCHORED_OBJECT_QA_PASSED", flush=True)
