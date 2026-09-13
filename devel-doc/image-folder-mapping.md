# Moved image folder mapping

In **Manage Images**, choose **Map Moved Folder...**. Select or enter the
original image folder, then choose its new location. The original folder need
not exist. Ancestors of missing links are offered so a whole collection can be
mapped at once.

For example, mapping `/old/assets` to `/new/assets` resolves
`/old/assets/print/logo.png` to `/new/assets/print/logo.png` and
`/old/assets/web/logo.png` to `/new/assets/web/logo.png` independently.

The search compares full relative paths. It prefers exact case, then tries a
case-insensitive match of the entire relative path. Multiple candidates require
selection in the existing preview dialog. It does not fall back to a filename
in another subfolder. Missing links outside the source folder are excluded.
Windows drive and UNC source paths are understood by the matching core on any
host. Links in a loaded document still use Scribus's existing path handling.

A review lists the original links, proposed replacements and unresolved links.
Cancel leaves the document unchanged. Applying the review uses the existing
failure-safe relinker and a single undo transaction. Shared links are applied
to every missing frame using that link; available images and embedded images
are excluded. The existing relinker retains crop, scale and image settings and
preserves the original link if loading fails. Saving the document persists the
resulting links; folder mappings themselves are not stored as automatic rules.

## Verification

The matcher tests cover duplicate filenames in different subfolders, source
folder boundaries, parent traversal, missing relative paths, case fallback,
Unix paths, Windows drive paths and UNC shares. They run in both the full build
and the portable test suite. Existing image-relink integration tests cover
failure rollback, image scale/crop, undo/redo and SLA save/reopen.

For this slice the macOS ARM64 build uses Qt 6.11.1. The portable suite and
Windows project-source audit were run on macOS; these are not native Windows
or Linux application certification. Native application and GUI validation of
this command remain to be performed on Windows and Linux. The new dialog flow
also needs interactive smoke testing on macOS; automated core tests do not
exercise its buttons or the batch undo transaction through the UI.

## Interactive smoke test

1. Place images from `assets/print/logo.png` and `assets/web/logo.png`, with
   visibly different content. Use one link in several frames, and change crop
   and scale on a frame.
2. Save and close the document, move `assets` elsewhere and reopen it with
   missing links. Include an unrelated missing image outside `assets`.
3. Map the old `assets` folder to the new one. Verify the review keeps the
   `print` and `web` paths distinct and excludes the unrelated link.
4. Cancel the review, verify nothing changed, then repeat and apply. Verify
   every frame with a shared missing link updates and retains its settings.
5. Close Manage Images. Undo once, then redo once; verify the entire batch.
   Save and reopen to check persistence.
6. Repeat with a missing replacement, a corrupt replacement and a cancelled
   scan. Check that unresolved links and available/embedded images are retained.
7. On a filesystem supporting case-distinct names, create two relative paths
   differing only in case and request a third casing. Verify the ambiguity
   preview appears, and that Skip leaves that link unresolved.

GPL notices in the implementation and existing build registrations are retained.
