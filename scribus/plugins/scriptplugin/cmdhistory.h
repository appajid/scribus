/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef CMDHISTORY_H
#define CMDHISTORY_H

// Pulls in <Python.h> first
#include "cmdvar.h"

PyDoc_STRVAR(scribus_undo__doc__,
QT_TR_NOOP("undo()\n\nUndo the most recent operation in the current document."));
PyObject *scribus_undo(PyObject* /* self */);

PyDoc_STRVAR(scribus_redo__doc__,
QT_TR_NOOP("redo()\n\nRedo the most recently undone operation in the current document."));
PyObject *scribus_redo(PyObject* /* self */);

#endif
