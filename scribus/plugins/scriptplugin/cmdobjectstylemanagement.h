/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef CMDOBJECTSTYLEMANAGEMENT_H
#define CMDOBJECTSTYLEMANAGEMENT_H

// Pulls in <Python.h> first
#include "cmdvar.h"

PyDoc_STRVAR(scribus_renameobjectstyle__doc__,
QT_TR_NOOP("renameObjectStyle(oldName, newName)\n\nRename a document Object Style and update assigned objects and dependent Object Styles. The operation can be undone and redone."));
PyObject *scribus_renameobjectstyle(PyObject* /* self */, PyObject* args);

PyDoc_STRVAR(scribus_deleteobjectstyle__doc__,
QT_TR_NOOP("deleteObjectStyle(name, [replacement])\n\nDelete a document Object Style. Assigned objects and dependent Object Styles use replacement when supplied; otherwise their association is cleared while their current appearance is preserved. The operation can be undone and redone."));
PyObject *scribus_deleteobjectstyle(PyObject* /* self */, PyObject* args);

#endif
