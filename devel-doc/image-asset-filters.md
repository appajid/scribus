# Image asset filters

For general Scribus copyright and licensing information, see the `COPYING` file
provided with the program.

Manage Images provides a non-destructive view over every image frame in the
document. The status selector supports:

- **All Images**: every non-LaTeX image frame.
- **Missing Links**: linked frames whose source image is unavailable.
- **Available Links**: linked frames whose source image is available.
- **Embedded Images**: frames whose image data is stored in the document.
- **Empty Frames**: image frames that do not yet have an image path.

The adjacent search field matches the displayed filename, the stored path using
either portable or native separators, and the frame name. Status and text filters
are combined. The result label always reports the visible and total counts.

Filters affect only the displayed list. **Map Moved Folder** and **Relink All
Missing Images** continue to operate on all eligible missing links in the
document; their tooltips state this explicitly. A hidden selection is replaced
with the first visible image. If nothing matches, image-specific controls are
cleared and disabled so an action cannot accidentally target an invisible frame.
Changing the filter or clearing the search restores a valid selection.

## Validation

The reusable fixture in
`scribus/tests/scripts/create_image_mapping_fixture.py` creates eight image
frames: six missing links, one available link, and one empty frame. Interactive
macOS QA verified those exact counts, filename/path/frame-name searches, the
searchable **Empty Image Frame** label, and the safe zero-result state. The same
dialog was also checked against a partially relinked fixture (three missing and
four available links). Embedded-image filtering is implemented but requires a
separate interactive embedded-image fixture for positive-path validation.

The dialog continues to use Qt widgets and portable `QDir` path handling; there
is no platform-specific code in this slice. Native Windows and Linux GUI smoke
tests remain part of release-candidate validation.
