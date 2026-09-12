/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "cmdhistory.h"
#include "cmdutil.h"

#include "undomanager.h"

PyObject *scribus_undo(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	UndoManager::instance()->undo(1);
	Py_RETURN_NONE;
}

PyObject *scribus_redo(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	UndoManager::instance()->redo(1);
	Py_RETURN_NONE;
}
