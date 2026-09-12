/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef CMDOBJECTSTYLEIMPORT_H
#define CMDOBJECTSTYLEIMPORT_H

// Pulls in <Python.h> first
#include "cmdvar.h"

PyDoc_STRVAR(scribus_importobjectstyles__doc__,
QT_TR_NOOP("importObjectStyles(filename, [styles, renameOnClash]) -> dict\n\nImport Object Styles from another Scribus document. styles may be a sequence of names or None for every style. Parent Object Styles, colors, and custom line styles are included automatically. renameOnClash defaults to True. The returned dictionary maps source style names to their destination names."));
PyObject *scribus_importobjectstyles(PyObject* /* self */, PyObject* args, PyObject* keywords);

#endif
